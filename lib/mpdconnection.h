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

#ifndef MPDCONNECTION_H
#define MPDCONNECTION_H

#include <QMutex>
#include <QThread>
#include <QTcpSocket>
#include <QDateTime>

#include "mpdstats.h"
#include "mpdstatus.h"
#include "song.h"

class MPDConnection : public QThread
{
	Q_OBJECT

	public:
		MPDConnection(QObject *parent = 0);
		MPDConnection(const QString &host, const quint16 port, QObject *parent = 0);
		~MPDConnection();
#if QT_VERSION < 0x040400
		void run();
#endif
		bool connectToMPD();
		void setHostname(const QString &host);
		void setPort(const quint16 port);
		bool isConnected();

		// Admin
		// TODO

		// Playlist
		void add(const QStringList &files);
		void addid(const QStringList &files, const int pos, const int size);
		void currentSong();
		void playListInfo();
		void removeSongs(const QList<qint32> &items);
		void move(const quint32 from, const quint32 to);
		void move(const QList<quint32> items, const quint32 diff, const int max);
		// TODO

		// Playback
		void setCrossfade(quint8 secs);
		void goToNext();
		void setPause(bool toggle);
		void startPlayingSong(quint32 song = 0);
		void startPlayingSongId(quint32 song_id = 0);
		void goToPrevious();
		void setRandom(bool toggle);
		void setRepeat(bool toggle);
		void setSeek(quint32 song, quint32 time);
		void setSeekId(quint32 song_id, quint32 time);
		void setVolume(quint8 vol);
		void stopPlaying();

		// Miscellaneous
		void clearError();
		void closeConnection();
		void getCommands();
		void getNotCommands();
		void setPassword(const QString &pass);
		void ping();

	public slots:
		// Playlist
		void clear();

		// Miscellaneous
		void getStats();
		void getStatus();

	protected:
		QString hostname;
		quint16 port;
		QString password;
		QTcpSocket sock;
		bool connected;
		QMutex mutex;

		bool commandOk();
		void sendCommand(const QByteArray &command);

	protected slots:
		QByteArray * readFromSocket();

	signals:
		void currentSongUpdated(const Song *song);
		void playlistUpdated(QList<Song *> *songs);
		void statsUpdated();
		void statusUpdated();
};

#endif
