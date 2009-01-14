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

#include "musiclibraryitemartist.h"
#include "musiclibraryitemalbum.h"
#include "musiclibraryitemsong.h"

MusicLibraryItemAlbum::MusicLibraryItemAlbum(const QString &data, MusicLibraryItem *parent)
	: MusicLibraryItem(data, MusicLibraryItem::Type_Album),
	  m_parentItem(static_cast<MusicLibraryItemArtist *>(parent))
{
}

MusicLibraryItemAlbum::~MusicLibraryItemAlbum()
{
	qDeleteAll(m_childItems);
}

void MusicLibraryItemAlbum::appendChild(MusicLibraryItem * const item)
{
	m_childItems.append(static_cast<MusicLibraryItemSong *>(item));
}

/**
 * Insert a new child item at a given place
 *
 * @param child The child item
 * @param place The place to insert the child item
 */
void MusicLibraryItemAlbum::insertChild(MusicLibraryItem * const child, const int place)
{
	m_childItems.insert(place, static_cast<MusicLibraryItemSong *>(child));
}

MusicLibraryItem * const MusicLibraryItemAlbum::child(int row) const
{
	return m_childItems.value(row);
}

int MusicLibraryItemAlbum::childCount() const
{
	return m_childItems.count();
}

MusicLibraryItem * const MusicLibraryItemAlbum::parent() const
{
	return m_parentItem;
}

int MusicLibraryItemAlbum::row() const
{
	return m_parentItem->m_childItems.indexOf(const_cast<MusicLibraryItemAlbum*>(this));
}

void MusicLibraryItemAlbum::clearChildren()
{
	qDeleteAll(m_childItems);
}
