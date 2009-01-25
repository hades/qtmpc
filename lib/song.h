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

#ifndef SONG_H
#define SONG_H

#include <QtGlobal>
#include <QObject>
#include <QString>

class Song : public QObject
{
	Q_OBJECT

	public:
		qint32 id;
		QString file;
		quint32 time;
		QString album;
		QString artist;
		QString title;
		qint32 track;
		quint32 pos;
		quint32 disc;

		Song();
		Song(const Song &);
		Song& operator=(const Song&);
		virtual ~Song();
		bool isEmpty() const;
		void fillEmptyFields();
		virtual void clear();
		static QString formattedTime(const quint32 &seconds);
};

class ScrobblingSong : public Song
{
	Q_OBJECT

	public:
		QString timePlayedAt;

		ScrobblingSong();
		ScrobblingSong(const ScrobblingSong &);
		ScrobblingSong& operator=(const ScrobblingSong&);
		void clear();
};

#endif
