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

#ifndef MPD_STATS_H
#define MPD_STATS_H

#include <QDateTime>
#include <QReadWriteLock>

class MPDStats
{
	public:
		static MPDStats * getInstance();

		static void acquireWriteLock();
		static void releaseWriteLock();

		// Getters
		static quint32 artists();
		static quint32 albums();
		static quint32 songs();
		static quint32 uptime();
		static quint32 playtime();
		static quint32 dbPlaytime();
		static QDateTime dbUpdate();
		static quint32 playlistArtists();
		static quint32 playlistAlbums();
		static quint32 playlistSongs();
		static quint32 playlistTime();

		// Setters
		static void setArtists(quint32 artists);
		static void setAlbums(quint32 albums);
		static void setSongs(quint32 songs);
		static void setUptime(quint32 uptime);
		static void setPlaytime(quint32 playtime);
		static void setDbPlaytime(quint32 db_playtime);
		static void setDbUpdate(uint seconds);
		static void setPlaylistArtists(quint32 artists);
		static void setPlaylistAlbums(quint32 albums);
		static void setPlaylistSongs(quint32 songs);
		static void setPlaylistTime(quint32 time);

	private:
		static quint32 m_artists;
		static quint32 m_albums;
		static quint32 m_songs;
		static quint32 m_uptime;
		static quint32 m_playtime;
		static quint32 m_db_playtime;
		static QDateTime m_db_update;
		static QReadWriteLock m_lock;
		static quint32 m_playlist_artists;
		static quint32 m_playlist_albums;
		static quint32 m_playlist_songs;
		static quint32 m_playlist_time;

		MPDStats() {}
		~MPDStats() {}
		MPDStats(const MPDStats&);
		MPDStats& operator=(const MPDStats& other);
};

#endif
