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

#include <QtCore>

#include "mpddatabaseconnection.h"
#include "mpdparseutils.h"

MPDDatabaseConnection::MPDDatabaseConnection(QObject *parent)
	: MPDConnection(parent)
{
}

MPDDatabaseConnection::MPDDatabaseConnection(const QString &host, const quint16 port,
QObject*parent)
	: MPDConnection(host, port, parent)
{
}

#if QT_VERSION < 0x040400
void MPDDatabaseConnection::run()
{
	exec();
}
#endif


/*
 * Admin commands
 */
void MPDDatabaseConnection::update()
{
	QByteArray *data;

	sendCommand("update");
	data = readFromSocket();

	delete data;
}

/*
 * Database commands
 */

/**
 * Get all files in the playlist with detailed info (artist, album,
 * title, time etc).
 *
 * @param db_update The last update time of the library
 */
void MPDDatabaseConnection::listAllInfo(QDateTime db_update)
{
	QByteArray *data;

	sendCommand("listallinfo");
	data = readFromSocket();

	emit musicLibraryUpdated(MPDParseUtils::parseLibraryItems(data), db_update);

	delete data;
}

/**
 * list info from a direction
 *
 * TODO: db_update should be parsed correctly
 */
void MPDDatabaseConnection::lsInfo()
{
	QByteArray *data;

	sendCommand("lsinfo");
	data = readFromSocket();

	emit musicLibraryUpdated(MPDParseUtils::parseLibraryItems(data), QDateTime());

	delete data;
}

/**
* Get all the files and dir in the mpdmusic dir.
*
*/
void MPDDatabaseConnection::listAll()
{
	QByteArray *data;
	
	sendCommand("listall");
	data = readFromSocket();
	
	emit dirViewUpdated(MPDParseUtils::parseDirViewItems(data));
	
	delete data;
}
