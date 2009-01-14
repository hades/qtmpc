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

#ifndef MPD_STATUS_H
#define MPD_STATUS_H

#include <QtCore>
#include <QtGlobal>


class MPDStatus
{
	public:
		enum State {
			State_Inactive,
			State_Playing,
			State_Stopped,
			State_Paused
		};

		static MPDStatus * getInstance();

		static void acquireWriteLock();
		static void releaseWriteLock();

		// Getters
		static quint8 volume();
		static bool repeat();
		static bool random();
		static quint32 playlist();
		static qint32 playlistLength();
		static qint32 playlistQueue();
		static qint32 xfade();
		static State state();
		static qint32 song();
		static qint32 songId();
		static qint32 timeElapsed();
		static qint32 timeTotal();
		static quint16 bitrate();
		static quint16 samplerate();
		static quint8 bits();
		static quint8 channels();
		static qint32 updatingDb();
		static QString error();

		// Setters
		static void setVolume(quint8 volume);
		static void setRepeat(bool repeat);
		static void setRandom(bool random);
		static void setPlaylist(quint32 playlist);
		static void setPlaylistLength(qint32 playlist_length);
		static void setPlaylistQueue(qint32 playlist_queue);
		static void setXfade(qint32 xfade);
		static void setState(State state);
		static void setSong(qint32 song);
		static void setSongId(qint32 song_id);
		static void setTimeElapsed(qint32 time_elapsed);
		static void setTimeTotal(qint32 time_total);
		static void setBitrate(quint16 bitrate);
		static void setSamplerate(quint16 samplerate);
		static void setBits(quint8 bits);
		static void setChannels(quint8 channels);
		static void setUpdatingDb(qint32 updating_db);
		static void setError(QString error);

	private:
		static quint8 m_volume;
		static bool m_repeat;
		static bool m_random;
		static quint32 m_playlist;
		static qint32 m_playlist_length;
		static qint32 m_playlist_queue;
		static qint32 m_xfade;
		static State m_state;
		static qint32 m_song;
		static qint32 m_song_id;
		static qint32 m_time_elapsed;
		static qint32 m_time_total;
		static quint16 m_bitrate;
		static quint16 m_samplerate;
		static quint8 m_bits;
		static quint8 m_channels;
		static qint32 m_updating_db;
		static QString m_error;
		static QReadWriteLock m_lock;

		MPDStatus() {}
		~MPDStatus() {}
		MPDStatus(const MPDStatus&);
		MPDStatus& operator=(const MPDStatus& other);
};

#endif
