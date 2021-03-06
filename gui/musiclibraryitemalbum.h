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

#ifndef MUSIC_LIBRARY_ITEM_ALBUM_H
#define MUSIC_LIBRARY_ITEM_ALBUM_H

#include <QList>
#include <QVariant>

#include "musiclibraryitem.h"

class MusicLibraryItemArtist;
class MusicLibraryItemSong;

class MusicLibraryItemAlbum : public MusicLibraryItem
{
	public:
		MusicLibraryItemAlbum(const QString &data, MusicLibraryItem *parent = 0);
		~MusicLibraryItemAlbum();

		void appendChild(MusicLibraryItem * const child);
		void insertChild(MusicLibraryItem * const child, const int place);

		MusicLibraryItem * const child(int row) const;
		int childCount() const;
		int row() const;
		MusicLibraryItem * const parent() const;
		void clearChildren();

	private:
		QList<MusicLibraryItemSong *> m_childItems;
		MusicLibraryItemArtist * const m_parentItem;

	friend class MusicLibraryItemSong;
};

#endif
