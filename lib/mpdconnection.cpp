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

#include <QtCore>

#include "mpdconnection.h"
#include "mpdparseutils.h"

MPDConnection::MPDConnection(QObject *parent)
	: QThread(parent),
		port(0), connected(false)
{
}

MPDConnection::MPDConnection(const QString &host, const quint16 port,
QObject*parent)
	: QThread(parent),
		hostname(host), port(port), connected(false)
{
}

MPDConnection::~MPDConnection()
{
	sock.disconnectFromHost();
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

void MPDConnection::setHostname(const QString &host)
{
	hostname = host;
}

void MPDConnection::setPort(const quint16 port)
{
	this->port = port;
}

bool MPDConnection::isConnected()
{
	return (sock.state() == QAbstractSocket::ConnectedState);
}

QByteArray * MPDConnection::readFromSocket()
{
	QByteArray *data = new QByteArray;

	while(true) {
		while(sock.bytesAvailable() == 0) {
			qDebug("MPDConnection: waiting for data.");
			if(sock.waitForReadyRead(5000)) {
				break;
			}
		}

		data->append(sock.readAll());

		if(data->endsWith("OK\n") || data->startsWith("OK") || data->startsWith("ACK")) {
			break;
		}
	}

	qDebug("Received: %s", data->constData());

	mutex.unlock();

	return data;
}

bool MPDConnection::connectToMPD()
{
	QByteArray senddata;
	QByteArray *recvdata;

	if(!isConnected()) {
		if(hostname.isEmpty() || port == 0) {
			qWarning("MPDConnection: no hostname and/or port supplied.");
			return false;
		}

		mutex.lock();
		sock.connectToHost(hostname, port);
		if(sock.waitForConnected(5000)) {
			qDebug("MPDConnection established");
			recvdata = readFromSocket();

			if(recvdata->startsWith("OK MPD")) {
				qDebug("Received identification string");
			}

			delete recvdata;

			if(!password.isEmpty()) {
				mutex.lock();
				qDebug("MPDConnection: setting password...");
				senddata = "password ";
				senddata += password.toUtf8();
				senddata += "\n";
				sock.write(senddata);
				sock.waitForBytesWritten(5000);

				if(commandOk()) {
					qDebug("MPDConnection: password accepted");
				} else {
					qDebug("MPDConnection: password rejected");
				}
			}

			return true;
		} else {
			mutex.unlock();
			qWarning("Couldn't connect");
			return false;
		}
	}

	qDebug("Already connected");
	return true;
}

bool MPDConnection::commandOk()
{
	QByteArray data;

	while(!sock.canReadLine()) {
		qDebug("MPDConnection: waiting for data.");
		if(sock.waitForReadyRead(5000)) {
			break;
		}
	}

	data += sock.readAll();

	mutex.unlock();

	if(data == "OK\n")
		return true;

	qWarning() << "MPD Error:" << data;

	return false;
}

void MPDConnection::sendCommand(const QByteArray &command)
{
	if(!connectToMPD()) {
		return;
	}

	mutex.lock();

	sock.write(command);
	sock.write("\n");
	sock.waitForBytesWritten(5000);
}


/*
 * Playlist commands
 */

void MPDConnection::add(const QStringList &files)
{
	QByteArray send = "command_list_begin\n";

	for(int i = 0; i < files.size(); i++) {
		send += "add \"";
		send += files.at(i).toUtf8();
		send += "\"\n";
	}

	send += "command_list_end";

	sendCommand(send);

	if(!commandOk())
		qDebug("Couldn't add song(s) to playlist");
}

/**
 * Add all the files in the QStringList to the playlist at
 * postition post.
 *
 * NOTE: addid is not fully supported in < 0.14 So add everything
 * and then move everything to the right spot.
 *
 * @param files A QStringList with all the files to add
 * @param pos Position to add the files
 * @param size The size of the current playlist
 */
void MPDConnection::addid(const QStringList &files, const int pos, const int size)
{
	QByteArray send = "command_list_begin\n";
	int cur_size = size;
	for(int i = 0; i < files.size(); i++) {
		send += "add \"";
		send += files.at(i).toUtf8();
		send += "\"";
		send += "\n";
		send += "move ";
		send += QByteArray::number(cur_size);
		send += " ";
		send += QByteArray::number(pos);
		send += "\n";
		cur_size++;
	}

	send += "command_list_end";

	sendCommand(send);

	if(!commandOk())
		qDebug("Couldn't add song(s) to playlist");
}

void MPDConnection::clear()
{
	sendCommand("clear");

	if(!commandOk())
		qDebug("Couldn't clear playlist");
}

void MPDConnection::removeSongs(const QList<qint32> &items)
{
	QByteArray send = "command_list_begin\n";

	for(int i = 0; i < items.size(); i++) {
		send += "deleteid ";
		send += QByteArray::number(items.at(i));
		send += "\n";
	}

	send += "command_list_end";

	sendCommand(send);

	if(!commandOk())
		qDebug("Couldn't remove songs from playlist");
}

void MPDConnection::move(const quint32 from, const quint32 to)
{
	QByteArray send = "move " + QByteArray::number(from) + " " + QByteArray::number(to);

	qWarning() << send;
	sendCommand(send);

	if(!commandOk())
		qDebug("Couldn't move files around in playlist");
}

void MPDConnection::move(const QList<quint32> items, const quint32 diff, const int max) {
	QByteArray send = "command_list_begin\n";

	quint32 times0 = 0;
	quint32 timesMax = 0;
	foreach (quint32 from, items) {
		send += "move ";
		if (((qint32)(from + diff)) <= 0) {
			send += QByteArray::number(from+times0);
			send += " ";
			send += "0";
			times0++;
		} else if ((qint32)(from + diff) >= max) {
			send += QByteArray::number(from-timesMax);
			send += " ";
			send += QByteArray::number(max);
			timesMax++;
		} else {
			send += QByteArray::number(from);
			send += " ";
			if (times0 > 0) {
				send += QByteArray::number(from + diff + times0 - 1);
			} else if (timesMax > 0) {
				send += QByteArray::number(from + diff - timesMax + 1);
			} else {
				send += QByteArray::number(from + diff);
			}
		}
		send += "\n";
	}

	send += "command_list_end";

	sendCommand(send);

	if (!commandOk())
		qDebug("Couldn't move songs around in playlist");
}

void MPDConnection::currentSong()
{
	QByteArray *data;

	sendCommand("currentsong");
	data = readFromSocket();

	emit currentSongUpdated(MPDParseUtils::parseSong(data));

	delete data;
}

void MPDConnection::playListInfo()
{
	QByteArray *data;

	sendCommand("playlistinfo");
	data = readFromSocket();

	emit playlistUpdated(MPDParseUtils::parseSongs(data));

	delete data;
}

/*
 * Playback commands
 */

void MPDConnection::setCrossfade(quint8 secs)
{
	QByteArray data = "crossfade ";
	data += QByteArray::number(secs);
	sendCommand(data);

	if(!commandOk())
		qDebug("Couldn't go to next track");
}

void MPDConnection::goToNext()
{
	sendCommand("next");

	if(!commandOk())
		qDebug("Couldn't go to next track");
}

void MPDConnection::setPause(bool toggle)
{
	QByteArray data = "pause ";
	if(toggle == true)
		data += "1";
	else
		data += "0";

	sendCommand(data);

	if(!commandOk())
		qDebug("Couldn't set pause");
}

void MPDConnection::startPlayingSong(quint32 song)
{
	QByteArray data = "play ";
	data += QByteArray::number(song);
	sendCommand(data);

	if(!commandOk())
		qDebug("Couldn't start playing song");
}

void MPDConnection::startPlayingSongId(quint32 song_id)
{
	QByteArray data = "playid ";
	data += QByteArray::number(song_id);
	sendCommand(data);

	if(!commandOk())
		qDebug("Couldn't start playing song id");
}

void MPDConnection::goToPrevious()
{
	sendCommand("previous");

	if(!commandOk())
		qDebug("Couldn't go to previous track");
}

void MPDConnection::setRandom(bool toggle)
{
	QByteArray data = "random ";
	if(toggle == true)
		data += "1";
	else
		data += "0";

	sendCommand(data);

	if(!commandOk())
		qDebug("Couldn't toggle random");
}

void MPDConnection::setRepeat(bool toggle)
{
	QByteArray data = "repeat ";
	if(toggle == true)
		data += "1";
	else
		data += "0";

	sendCommand(data);

	if(!commandOk())
		qDebug("Couldn't toggle repeat");
}

void MPDConnection::setSeek(quint32 song, quint32 time)
{
	QByteArray data = "seek ";
	data += QByteArray::number(song);
	data += " ";
	data += QByteArray::number(time);

	sendCommand(data);

	if(!commandOk())
		qDebug("Couldn't set seek position");
}

void MPDConnection::setSeekId(quint32 song_id, quint32 time)
{
	QByteArray data = "seekid ";
	data += QByteArray::number(song_id);
	data += " ";
	data += QByteArray::number(time);
	sendCommand(data);

	if(!commandOk())
		qDebug("Couldn't set seek position");
}

void MPDConnection::setVolume(quint8 vol)
{
	QByteArray data = "setvol ";
	data += QByteArray::number(vol);
	sendCommand(data);

	if(!commandOk())
		qDebug("Couldn't set volume");
}

void MPDConnection::stopPlaying()
{
	sendCommand("stop");

	if(!commandOk())
		qDebug("Couldn't stop playing");
}

/*
 * Miscellaneous commands
 */

void MPDConnection::clearError()
{
}

void MPDConnection::closeConnection()
{
}

void MPDConnection::getCommands()
{
}

void MPDConnection::getNotCommands()
{
}

void MPDConnection::setPassword(const QString &pass)
{
	this->password = pass;
}

void MPDConnection::ping()
{
}

void MPDConnection::getStats()
{
	QByteArray *data;

	sendCommand("stats");
	data = readFromSocket();
	MPDParseUtils::parseStats(data);

	emit statsUpdated();

	delete data;
}

void MPDConnection::getStatus()
{
	QByteArray *data;

	sendCommand("status");
	data = readFromSocket();
	MPDParseUtils::parseStatus(data);

	emit statusUpdated();

	delete data;
}
