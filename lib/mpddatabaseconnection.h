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

#ifndef MPDDATABASECONNECTION_H
#define MPDDATABASECONNECTION_H

#include "lib/mpdconnection.h"

class MusicLibraryItemArtist;
class DirViewItemRoot;

class MPDDatabaseConnection : public MPDConnection
{
	Q_OBJECT

	public:
		MPDDatabaseConnection(QObject *parent = 0);
		MPDDatabaseConnection(const QString &host, const quint16 port, QObject *parent = 0);
#if QT_VERSION < 0x040400
		void run();
#endif

		// Database
		void listAllInfo(QDateTime db_update);
		void lsInfo();
		void listAll();
		// TODO

	public slots:
		// Admin
		void update();

	signals:
		void musicLibraryUpdated(QList<MusicLibraryItemArtist *> *items, QDateTime db_update);
		void dirViewUpdated(DirViewItemRoot * root);
};

#endif
