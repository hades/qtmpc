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

#include <cmath>
#include "song.h"

Song::Song()
	: QObject(),
	  id(-1),
	  time(0),
	  track(0),
	  pos(0),
	  disc(0)
{
}

Song::Song(const Song &s)
	: QObject(),
	  id(s.id),
	  file(s.file),
	  time(s.time),
	  album(s.album),
	  artist(s.artist),
	  title(s.title),
	  track(s.track),
	  pos(s.pos),
	  disc(s.disc)
{
}

Song& Song::operator=(const Song &s)
{
	id = s.id;
	file = s.file;
	time = s.time;
	album = s.album;
	artist = s.artist;
	title = s.title;
	track = s.track;
	pos = s.pos;
	disc = s.disc;

	return *this;
}

Song::~Song()
{
}

bool Song::isEmpty() const
{
	return
	(
		(artist.isEmpty() && album.isEmpty() && title.isEmpty())
		|| file.isEmpty()
	);
}

void Song::fillEmptyFields()
{
	if(artist.isEmpty())
		artist = "<Unknown>";

	if(album.isEmpty())
		album = "<Unknown>";

	if(title.isEmpty())
		title = "<Unknown>";
}

void Song::clear()
{
	id = -1;
	file.clear();
	time = 0;
	album.clear();
	artist.clear();
	title.clear();
	track = 0;
	pos = 0;
	disc = 0;
}

QString Song::formattedTime(const quint32 &seconds)
{
	QString result;

	result += QString::number(floor(seconds / 60.0));
	result += ":";
	if(seconds % 60 < 10)
		result += "0";
	result += QString::number(seconds % 60);

	return result;
}

ScrobblingSong::ScrobblingSong() : Song()
{
}

ScrobblingSong::ScrobblingSong(const ScrobblingSong &s) : Song(s), timePlayedAt(s.timePlayedAt)
{
}

void ScrobblingSong::clear()
{
	Song::clear();
	timePlayedAt.clear();
}
