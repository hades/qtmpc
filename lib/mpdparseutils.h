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

#ifndef MPD_PARSE_UTILS_H
#define MPD_PARSE_UTILS_H

#include <QString>


class MusicLibraryItemArtist;
class Song;
class DirViewItemRoot;

class MPDParseUtils
{
	public:
		static void parseStats(const QByteArray * const data);
		static void parseStatus(const QByteArray * const data);
		static Song * parseSong(const QByteArray * const data);
		static QList<Song *> * parseSongs(const QByteArray * const data);
		static QList<MusicLibraryItemArtist *> * parseLibraryItems(const QByteArray * const data);
		static DirViewItemRoot * parseDirViewItems(const QByteArray * const data);
		static QString seconds2formattedString(const quint32 totalseconds);
};

#endif
