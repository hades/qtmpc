/*
 * Copyright (c) 2008 Sander Knopper (sander AT knopper DOT tk) and
 *                    Roeland Douma (roeland AT rullzer DOT com)
 *
 * This file is part of QtMPC.
 *
 * QtMPC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * QtMPC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QtMPC.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QString>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QCryptographicHash>
#include <QMap>
#include <QMapIterator>
#include <QSettings>
#include <QDateTime>
#include <QBuffer>
#include <QUrl>
#include <QHttpRequestHeader>
#include <QFile>
#include <QDir>
#include <QMutexLocker>
#include <QMutex>

#include "lastfm_scrobbling.h"

#include <QDebug>

LastFmScrobbler::LastFmScrobbler(QObject *parent)
	: QThread(parent),
		startPlayingDateTime(QDateTime::currentDateTime()),
		timePlayed(0),
		enabled(false),
		failures(0),
		hs_failed(false),
		hs_timestamp(0),
		api_key("11172d35eb8cc2fd33250a9e45a2d486"),
		secret_key("f551359ab7f6d759eb1880f554e5e815")
{
	current_song.length = 0;
	fromXML();
}

/**
 * When we are closing down write all unsubmitted songs to xml
 */
LastFmScrobbler::~LastFmScrobbler()
{
	toXML();
	quit();
	
	while(isRunning())
		msleep(100);
}

#if QT_VERSION < 0x040400
void MPDConnection::run()
{
	exec();
}
#endif

/**
 * Create a basic api signature
 * see: http://www.last.fm/api/authspec#8
 */
QString LastFmScrobbler::apiSig(QMap<QString, QString> args)
{
	QMapIterator<QString, QString> i(args);
	QString string;

	while(i.hasNext()) {
		i.next();
		string += i.key();
		string += i.value();
	}
	string += secret_key;

	return QCryptographicHash::hash(string.toUtf8(), QCryptographicHash::Md5).toHex();
}

/**
 * Generate auth token for authentication
 * see: http://www.last.fm/api/mobileauth
 *
 * md5(usename + md5(password))
 */
QString LastFmScrobbler::generateAuthToken()
{
	QSettings settings;
	QString username = settings.value("lastfm/username", "").toString().toLower();
	QString password = settings.value("lastfm/password", "").toString();

	QString passwordHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Md5).toHex();
	QString hash = username.toUtf8() + passwordHash;
	hash = QCryptographicHash::hash(hash.toUtf8(), QCryptographicHash::Md5).toHex();

	return hash;
}

/**
 * Generate an auttoken for the handsake
 * see: http://www.last.fm/api/submissions#handshake
 *
 * md5(shared_secret + timestamp)
 */
QString LastFmScrobbler::generateAuthTokenHandShake(QString secret, QString timestamp)
{
	QString hash = secret + timestamp;
	hash = QCryptographicHash::hash(hash.toUtf8(), QCryptographicHash::Md5).toHex();

	return hash;
}

/**
 * Generate a url: key[0]=value[0]&key[1]=value[1]&...
 */
QString LastFmScrobbler::generateURL(QMap<QString, QString> args)
{
	QMapIterator<QString, QString> i(args);
	QString url = "";

	while(i.hasNext()) {
		i.next();
		url += i.key();
		url += "=";
		url += i.value();
		if (i.hasNext())
			url += "&";
	}

	return url;
}

/**
 * Get a session key for a user. This session is valid for life so store it!
 */
bool LastFmScrobbler::getSessionKey()
{
	QSettings settings;
	http.setHost("ws.audioscrobbler.com");

	QString start_path = "/2.0/?";

	QMap<QString, QString> args;
	args["method"] = "auth.getmobilesession";
	args["api_key"] = api_key;
	args["authToken"] = generateAuthToken();
	args["username"] = settings.value("lastfm/username", "").toString();
	args["api_sig"] = apiSig(args);

	QString path = generateURL(args);
	QBuffer output;
	http.syncGet(start_path+path, &output);

	QString session(output.data());
	QXmlStreamReader doc(session);
	bool fault = true;
	QString key = "";
	while(!doc.atEnd()) {
		doc.readNext();
		if(doc.isStartElement() && doc.name() == "lfm") {
			if (doc.attributes().value("status").toString() == "ok") {
				fault = false;
			} else {
				emit authFailed(1);
				return false;
			}
		}
		if (!fault && doc.isStartElement() && doc.name() == "key") {
			key = doc.readElementText();
		}
	}
	if (key != "") {
		settings.setValue("lastfm/key", key);
		return true;
	}
	return false;
}

/**
 * Lets shake!
 * see: http://www.last.fm/api/submissions#handshake
 *
 * TODO: Better error handling
 */
bool LastFmScrobbler::handShake()
{
	//Check if we need to wait!
	if (hs_failed) {
		uint min = 1;
		for (int i = 1; i < hs_failures; i++) {
			min *= 2;
		}
		if ((QDateTime::currentDateTime().toTime_t() - hs_timestamp) < (min * 60))
			return false;
	}

	QSettings settings;
	if (settings.value("lastfm/key", "").toString() == "") {
		if (!getSessionKey()) {
			return false;
		}
	}

	http.setHost("post.audioscrobbler.com");
	QMap<QString, QString> args;
	uint time = QDateTime::currentDateTime().toTime_t();
	QString timestamp = QString::number(time);

	QString username = settings.value("lastfm/username", "").toString();
	QString password = settings.value("lastfm/password", "").toString();

	args["hs"] = "true";
	args["p"] = "1.2.1";
	args["c"] = "qtm";
	args["v"] = "0.4.1-svn";
	args["u"] = username;
	args["t"] = timestamp;
	args["a"] = generateAuthTokenHandShake(secret_key, timestamp);
	args["api_key"] = api_key;
	args["sk"] = settings.value("lastfm/key", "").toString();

	QString path = "/?" + generateURL(args);

	QBuffer output;
	if (!http.syncGet(path, &output)) {
		hs_failed = true;
		hs_failures++;
		hs_timestamp = QDateTime::currentDateTime().toTime_t();
		return false;
	}

	QString data(output.data());
	QStringList lines = data.split("\n", QString::SkipEmptyParts);

	if (lines.at(0) != "OK") {
		if (lines.at(0) == "BADTIME") {
			qWarning() << "Your system clock is wrong";
		} else if (lines.at(0) == "BADAUTH") {
			qWarning() << "Invalid authentication details";
			emit authFailed(1);
		} else if (lines.at(0) == "BANNED") {
			qWarning() << "Client is banned!";
		} else {
			hs_failed = true;
			hs_failures++;
			hs_timestamp = QDateTime::currentDateTime().toTime_t();
		}
		return false;
	}

	sessionID = lines.at(1);
	nowPlayingURL = lines.at(2);
	submissionURL = lines.at(3);
	hs_failed = false;
	hs_failures = 0;
	return true;
}

/**
 * Submit now playing to last.fm
 * see: http://www.last.fm/api/submissions#np
 */
void LastFmScrobbler::nowPlaying(QString artist, QString album, QString title, quint32 track, quint32 length)
{
	checkForSubmission();

	//First add the song
	current_song.artist = QUrl::toPercentEncoding(artist.toUtf8());
	current_song.title = QUrl::toPercentEncoding(title.toUtf8());
	current_song.album = QUrl::toPercentEncoding(album.toUtf8());
	current_song.length = length;
	current_song.track = track;
	current_song.current_time = QString::number(QDateTime::currentDateTime().toTime_t());

	while(true) {
		//Try to add to nowPlaying
		if (nowPlayingURL == "") {
			if (!handShake()) {
				return;
			}
		}

		QMap<QString, QString> args;
		args["s"] = sessionID;
		args["a"] = current_song.artist;
		args["t"] = current_song.title;
		args["b"] = current_song.album;
		args["i"] = QString::number(current_song.length);
		args["n"] = QString::number(current_song.track);

		QBuffer buffer;
		buffer.setData(generateURL(args).toUtf8());

		QStringList lines = nowPlayingURL.split("/", QString::SkipEmptyParts);
		QStringList host = lines.at(1).split(":", QString::SkipEmptyParts);
		http.setHost(host.at(0), host.at(1).toInt());

		QString path = "/" + lines.at(2);
		QBuffer output;

		QHttpRequestHeader header("POST", path);
		header.setValue("content-type", "application/x-www-form-urlencoded");
		header.setValue("User-Agent", "QtMPC(svn)");
		header.setValue("Host", host.at(0));

		if (!http.syncRequest(header, &buffer, &output)) {
			qDebug() << "Http Error";
			if (!handShake())
				break;
			continue;
		}

		QString data(output.data());
		QStringList output_lines = data.split("\n", QString::SkipEmptyParts);

		/*
		* We can get a OK or a BADSESSION.
		* On BADSESSION do a new handshake and resend.
		*
		* TODO: Do this more neat
		*/
		if (output_lines.at(0) == "BADSESSION") {
			qDebug() << "BADSESSION resend";
			if (!handShake())
				break;
		} else if (output_lines.at(0) == "OK") {
			break;
		}
	}
}

/**
 * Submit played track to last.fm
 * see: http://www.last.fm/api/submissions#subs
 */
void LastFmScrobbler::submission()
{
	if (submissionURL == "") {
		if (!handShake())
			return;
	}

	while (songs.size() > 0) {
		QString args = "";
		args += "s=" + sessionID;

		int size = songs.size() > 50 ? 50 : songs.size();
		for (int i = 0; i < size; i++) {
			song s = songs.at(i);
			QString num = QString::number(i);
			args += "&a[" + num + "]=" + s.artist;
			args += "&t[" + num + "]=" + s.title;
			args += "&b[" + num + "]=" + s.album;
			args += "&i[" + num + "]=" + s.current_time;
			args += "&l[" + num + "]=" + QString::number(s.length);
			args += "&n[" + num + "]=" + QString::number(s.track);
			args += "&o[" + num + "]=P";
			args += "&r[" + num + "]=";
			args += "&m[" + num + "]=";
		}

		QBuffer buffer;
		buffer.setData(args.toUtf8());

		QStringList lines = submissionURL.split("/", QString::SkipEmptyParts);
		QStringList host = lines.at(1).split(":", QString::SkipEmptyParts);
		http.setHost(host.at(0), host.at(1).toInt());

		QString path = "/" + lines.at(2);

		QHttpRequestHeader header("POST", path);
		header.setValue("content-type", "application/x-www-form-urlencoded");
		header.setValue("User-Agent", "QtMPC(svn)");
		header.setValue("Host", host.at(0));

		QBuffer output;
		if (!http.syncRequest(header, &buffer, &output)) {
			failures++;
			if (failures > 2)
				handShake();
			failures = 0;
			return;
		}

		QString data(output.data());
		QStringList output_lines = data.split("\n", QString::SkipEmptyParts);

		if (output_lines.at(0) == "OK") {
			for (int i = 0; i < size; i++) {
				songs.pop_front();
			}
		} else if (output_lines.at(0) == "BADSESSION") {
			handShake();
		} else {
			failures++;
			if (failures > 2)
				handShake();
			failures = 0;
			return;
		}
	}
}

/**
 * Pause scrobbling timer
 */
void LastFmScrobbler::pauseScrobblerTimer()
{
	QMutexLocker locker(&songs_lock);
	timePlayed = timePlayed + (QDateTime::currentDateTime().toTime_t() - startPlayingDateTime.toTime_t());
}

/**
 * Resume scrobbling timer
 */
void LastFmScrobbler::resumeScrobblerTimer()
{
	QMutexLocker locker(&songs_lock);
	startPlayingDateTime = QDateTime::currentDateTime();
}

/**
 * Start a new scrobbling timer
 */
void LastFmScrobbler::startScrobbleTimer()
{
	QMutexLocker locker(&songs_lock);
	timePlayed = 0;
	startPlayingDateTime = QDateTime::currentDateTime();
}

void LastFmScrobbler::stopScrobblerTimer()
{
	checkForSubmission();
	QMutexLocker locker(&songs_lock);
	timePlayed = timePlayed + (QDateTime::currentDateTime().toTime_t() - startPlayingDateTime.toTime_t());
	startPlayingDateTime = QDateTime::currentDateTime();

	timePlayed = 0;
}

/**
 * Check if we comply to submission demands
 * see: http://www.last.fm/api/submissions#3.1
 */
void LastFmScrobbler::checkForSubmission()
{
	QMutexLocker locker(&songs_lock);

	if (current_song.length == 0) {
		return;
	}

	timePlayed = timePlayed + (QDateTime::currentDateTime().toTime_t() - startPlayingDateTime.toTime_t());

	if (timePlayed > 240 || timePlayed > current_song.length/2) {
		songs.append(current_song);
		submission();
	}
}

void LastFmScrobbler::clearAuth(bool enabled)
{
	sessionID = "";
	nowPlayingURL = "";
	submissionURL = "";
	QSettings settings;
	settings.setValue("lastfm/key", "");

	if (enabled)
		handShake();
}

/**
 * Write all our unsubmitted songs to a xml file.
 */
void LastFmScrobbler::toXML()
{
	//Check if dir exists
	QDir savedir(QDir::home());
	if(!savedir.exists(".QtMPC")) {
		if(!savedir.mkdir(".QtMPC")) {
			qWarning("Couldn't create directory for storing lastfm file");
			return;
		}
	}

	//Create the filename
	QString dir(QDir::toNativeSeparators(QDir::homePath()) + QDir::separator() + ".QtMPC" + QDir::separator());
	QString filename(QFile::encodeName("lastfm.xml"));

	//Open the file
	QFile file(dir + filename);
	file.open(QIODevice::WriteOnly);

	QXmlStreamWriter writer(&file);
	writer.setAutoFormatting(true);
	writer.writeStartDocument();

	writer.writeStartElement("qtmpc_lastfm");
	writer.writeAttribute("version", "1");

	for (int i = 0; i < songs.size(); i++) {
		song s = songs.at(i);
		writer.writeEmptyElement("Song");
		writer.writeAttribute("artist", s.artist);
		writer.writeAttribute("album", s.album);
		writer.writeAttribute("title", s.title);
		writer.writeAttribute("track", QString::number(s.track));
		writer.writeAttribute("length", QString::number(s.length));
		writer.writeAttribute("current_time", s.current_time);
	}

	writer.writeEndElement();
	writer.writeEndDocument();
	file.close();
}

/**
 * Read a last.fm xml database file with unsubmitted songs
 */
void LastFmScrobbler::fromXML()
{
	QString dir(QDir::toNativeSeparators(QDir::homePath()) + QDir::separator() + ".QtMPC" + QDir::separator());
	QString filename(QFile::encodeName("lastfm.xml"));
	
	//Check if file exists
	if (!QFile::exists(dir + filename))
		return;
	
	QFile file(dir+filename);
	file.open(QIODevice::ReadOnly);

	QXmlStreamReader reader(&file);
	while(!reader.atEnd()) {
		reader.readNext();
		if (reader.error()){
			qDebug() << reader.errorString();
		} else {
			//Found Song... add it
			if (reader.isStartElement() && reader.name().toString() == "Song") {
				song s;
				s.artist = reader.attributes().value("artist").toString();
				s.album = reader.attributes().value("album").toString();
				s.title = reader.attributes().value("title").toString();
				s.track = reader.attributes().value("track").toString().toUInt();
				s.length = reader.attributes().value("length").toString().toUInt();
				s.current_time = reader.attributes().value("current_time").toString();

				songs.append(s);
			}
		}
	}
}
