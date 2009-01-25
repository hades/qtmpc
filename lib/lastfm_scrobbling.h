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

#ifndef LASTFM_SCROBBLING_H
#define LASTFM_SCROBBLING_H

#include <QImage>
#include <QString>
#include <QMap>
#include <QTimer>
#include <QDateTime>
#include <QList>
#include <QMutex>
#include <QThread>

#include "external/synchttp/synchttp.h"
#include "lib/song.h"

class LastFmScrobbler : public QThread
{
	Q_OBJECT

	public:
		LastFmScrobbler(QObject *parent = 0);
		~LastFmScrobbler();
		void setCurrentTrack(QString artist, QString album, QString title, quint32 track, quint32 length);

#if QT_VERSION < 0x040400
		void run();
#endif

	public slots:
		void clearAuth(bool enabled);
		void startScrobbleTimer();
		void pauseScrobblerTimer();
		void resumeScrobblerTimer();
		void stopScrobblerTimer();
		void nowPlaying();

	signals:
		void authFailed(const int);

	private:
		ScrobblingSong m_currentSong;
		QList<ScrobblingSong> m_songs;
		QMutex m_songsMutex;

		SyncHTTP m_http;
		QDateTime m_startPlayingDateTime;
		quint32 m_timePlayed;
		bool m_enabled;
		int m_failures;
		bool hs_failed;
		uint hs_timestamp;
		int hs_failures;

		const QString m_api_key;
		const QString m_secret_key;
		QString m_sessionID;
		QString m_nowPlayingURL;
		QString m_submissionURL;

		QString generateURL(QMap<QString, QString> args);
		QString apiSig(QMap<QString, QString> args);
		QString generateAuthToken();
		QString generateAuthTokenHandShake(QString secret, QString timestamp);

		void submission();
		bool handShake();
		bool getSessionKey();

		void toXML();
		void fromXML();
};

#endif
