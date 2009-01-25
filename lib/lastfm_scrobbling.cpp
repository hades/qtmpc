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

#include "QtMPC_config.h"
#include "lastfm_scrobbling.h"

#include <QDebug>

LastFmScrobbler::LastFmScrobbler(QObject *parent)
	: QThread(parent),
		m_startPlayingDateTime(QDateTime::currentDateTime()),
		m_timePlayed(0),
		m_enabled(false),
		m_failures(0),
		hs_failed(false),
		hs_timestamp(0),
		m_api_key("11172d35eb8cc2fd33250a9e45a2d486"),
		m_secret_key("f551359ab7f6d759eb1880f554e5e815")
{
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
	string += m_secret_key;

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
	return QString(QCryptographicHash::hash(secret.toUtf8() + timestamp.toUtf8(),
	                                        QCryptographicHash::Md5).toHex());
}

/**
 * Generate a url: key[0]=value[0]&key[1]=value[1]&...
 */
QString LastFmScrobbler::generateURL(QMap<QString, QString> args)
{
	QMapIterator<QString, QString> i(args);
	QString url;

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
	m_http.setHost("ws.audioscrobbler.com");

	QString start_path = "/2.0/?";

	QMap<QString, QString> args;
	args["method"] = "auth.getmobilesession";
	args["api_key"] = m_api_key;
	args["authToken"] = generateAuthToken();
	args["username"] = settings.value("lastfm/username", "").toString();
	args["api_sig"] = apiSig(args);

	QString path = generateURL(args);
	QBuffer output;
	m_http.syncGet(start_path+path, &output);

	QString session(output.data());
	QXmlStreamReader doc(session);
	bool fault = true;
	QString key;
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
	if (!key.isEmpty()) {
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
	if (settings.value("lastfm/key", "").toString().isEmpty()) {
		if (!getSessionKey()) {
			return false;
		}
	}

	m_http.setHost("post.audioscrobbler.com");
	QMap<QString, QString> args;
	uint time = QDateTime::currentDateTime().toTime_t();
	QString timestamp = QString::number(time);

	QString username = settings.value("lastfm/username", "").toString();
	QString password = settings.value("lastfm/password", "").toString();

	args["hs"] = "true";
	args["p"] = "1.2.1";
	args["c"] = "qtm";
	args["v"] = PACKAGE_VERSION;
	args["u"] = username;
	args["t"] = timestamp;
	args["a"] = generateAuthTokenHandShake(m_secret_key, timestamp);
	args["api_key"] = m_api_key;
	args["sk"] = settings.value("lastfm/key", "").toString();

	QString path = "/?" + generateURL(args);

	QBuffer output;
	if (!m_http.syncGet(path, &output)) {
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

	m_sessionID = lines.at(1);
	m_nowPlayingURL = lines.at(2);
	m_submissionURL = lines.at(3);
	hs_failed = false;
	hs_failures = 0;
	return true;
}

void LastFmScrobbler::setCurrentTrack(QString artist, QString album, QString title, quint32 track, quint32 length)
{
	QMutexLocker locker(&m_songsMutex);
	if (m_startPlayingDateTime.isValid()) {
		m_timePlayed = m_timePlayed + (QDateTime::currentDateTime().toTime_t()
		                - m_startPlayingDateTime.toTime_t());

		if (m_currentSong.time > 30 && (m_timePlayed > 240 || m_timePlayed > m_currentSong.time / 2)) {
			m_songs.append(m_currentSong);
		}
	}

	m_currentSong.artist = QUrl::toPercentEncoding(artist.toUtf8());
	m_currentSong.title = QUrl::toPercentEncoding(title.toUtf8());
	m_currentSong.album = QUrl::toPercentEncoding(album.toUtf8());
	m_currentSong.time = length;
	m_currentSong.track = track;
	m_currentSong.timePlayedAt = QString::number(QDateTime::currentDateTime().toTime_t());

	locker.unlock();
	startScrobbleTimer();
	submission();
}

/**
 * Submit now playing to last.fm
 * see: http://www.last.fm/api/submissions#np
 */
void LastFmScrobbler::nowPlaying()
{
	QMutexLocker locker(&m_songsMutex);
	//First add the song
	while(true) {
		//Try to add to nowPlaying
		if (m_nowPlayingURL.isEmpty()) {
			if (!handShake()) {
				return;
			}
		}

		QMap<QString, QString> args;
		args["s"] = m_sessionID;
		args["a"] = m_currentSong.artist;
		args["t"] = m_currentSong.title;
		args["b"] = m_currentSong.album;
		args["i"] = QString::number(m_currentSong.time);
		args["n"] = QString::number(m_currentSong.track);

		QBuffer buffer;
		buffer.setData(generateURL(args).toUtf8());

		QStringList lines = m_nowPlayingURL.split("/", QString::SkipEmptyParts);
		QStringList host = lines.at(1).split(":", QString::SkipEmptyParts);
		m_http.setHost(host.at(0), host.at(1).toInt());

		QString path = "/" + lines.at(2);
		QBuffer output;

		QHttpRequestHeader header("POST", path);
		header.setValue("content-type", "application/x-www-form-urlencoded");
		header.setValue("User-Agent", "QtMPC(svn)");
		header.setValue("Host", host.at(0));

		if (!m_http.syncRequest(header, &buffer, &output)) {
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
	if (m_submissionURL.isEmpty()) {
		if (!handShake())
			return;
	}

	QMutexLocker locker(&m_songsMutex);
	while (m_songs.size() > 0) {
		QString args = "s=" + m_sessionID;

		int size = m_songs.size() > 50 ? 50 : m_songs.size();
		for (int i = 0; i < size; i++) {
			ScrobblingSong s = m_songs.at(i);
			QString num = QString::number(i);
			args += "&a[" + num + "]=" + s.artist;
			args += "&t[" + num + "]=" + s.title;
			args += "&b[" + num + "]=" + s.album;
			args += "&i[" + num + "]=" + s.timePlayedAt;
			args += "&l[" + num + "]=" + QString::number(s.time);
			args += "&n[" + num + "]=" + QString::number(s.track);
			args += "&o[" + num + "]=P";
			args += "&r[" + num + "]=";
			args += "&m[" + num + "]=";
		}

		QBuffer buffer;
		buffer.setData(args.toUtf8());

		QStringList lines = m_submissionURL.split("/", QString::SkipEmptyParts);
		QStringList host = lines.at(1).split(":", QString::SkipEmptyParts);
		m_http.setHost(host.at(0), host.at(1).toInt());

		QString path = "/" + lines.at(2);

		QHttpRequestHeader header("POST", path);
		header.setValue("content-type", "application/x-www-form-urlencoded");
		header.setValue("User-Agent", PACKAGE_STRING);
		header.setValue("Host", host.at(0));

		QBuffer output;
		if (!m_http.syncRequest(header, &buffer, &output)) {
			m_failures++;
			if (m_failures > 2)
				handShake();
			m_failures = 0;
			return;
		}

		QString data(output.data());
		QStringList output_lines = data.split("\n", QString::SkipEmptyParts);

		if (output_lines.at(0) == "OK") {
			for (int i = 0; i < size; i++) {
				m_songs.pop_front();
			}
		} else if (output_lines.at(0) == "BADSESSION") {
			handShake();
		} else {
			m_failures++;
			if (m_failures > 2)
				handShake();
			m_failures = 0;
			return;
		}
	}
}

/**
 * Pause scrobbling timer
 */
void LastFmScrobbler::pauseScrobblerTimer()
{
	QMutexLocker locker(&m_songsMutex);
	m_timePlayed = m_timePlayed + (QDateTime::currentDateTime().toTime_t()
                    - m_startPlayingDateTime.toTime_t());
	m_startPlayingDateTime = QDateTime();
}

/**
 * Resume scrobbling timer
 */
void LastFmScrobbler::resumeScrobblerTimer()
{
	QMutexLocker locker(&m_songsMutex);
	m_startPlayingDateTime = QDateTime::currentDateTime();
}

/**
 * Start a new scrobbling timer
 */
void LastFmScrobbler::startScrobbleTimer()
{
	QMutexLocker locker(&m_songsMutex);
	m_timePlayed = 0;
	m_startPlayingDateTime = QDateTime::currentDateTime();
}

void LastFmScrobbler::stopScrobblerTimer()
{
	QMutexLocker locker(&m_songsMutex);
	m_timePlayed = m_timePlayed + (QDateTime::currentDateTime().toTime_t()
                    - m_startPlayingDateTime.toTime_t());
	m_startPlayingDateTime = QDateTime();

	if (m_timePlayed > 240 || m_timePlayed > m_currentSong.time / 2) {
		m_songs.append(m_currentSong);
	}

	m_timePlayed = 0;
	m_currentSong.clear();

	locker.unlock();
	submission();
}

void LastFmScrobbler::clearAuth(bool enabled)
{
	m_sessionID.clear();
	m_nowPlayingURL.clear();
	m_submissionURL.clear();
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

	for (int i = 0; i < m_songs.size(); i++) {
		ScrobblingSong s = m_songs.at(i);
		writer.writeEmptyElement("Song");
		writer.writeAttribute("artist", s.artist);
		writer.writeAttribute("album", s.album);
		writer.writeAttribute("title", s.title);
		writer.writeAttribute("track", QString::number(s.track));
		writer.writeAttribute("length", QString::number(s.time));
		writer.writeAttribute("current_time", s.timePlayedAt);
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
		if (reader.error()) {
			qDebug() << reader.errorString();
		} else {
			//Found Song... add it
			if (reader.isStartElement() && reader.name().toString() == "Song") {
				ScrobblingSong s;
				s.artist = reader.attributes().value("artist").toString();
				s.album = reader.attributes().value("album").toString();
				s.title = reader.attributes().value("title").toString();
				s.track = reader.attributes().value("track").toString().toUInt();
				s.time = reader.attributes().value("length").toString().toUInt();
				s.timePlayedAt = reader.attributes().value("current_time").toString();

				m_songs.append(s);
			}
		}
	}
}
