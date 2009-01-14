/***************************************************************************
 *   Copyright (C) 2005 by Iulian M                                        *
 *   eti@erata.net                                                         *
 ***************************************************************************/

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

#include "synchttp.h"

SyncHTTP::SyncHTTP( QObject * parent)
	:QHttp(parent),
		requestID(-1),
		status(false)
{
}

SyncHTTP::SyncHTTP( const QString & hostName, quint16 port, QObject * parent)
	:QHttp(hostName,port,parent),
		requestID(-1),
		status(false)
{
}

bool SyncHTTP::syncGet ( const QString & path, QIODevice * to)
{
	mutex.lock();
	///connect the requestFinished signal to our finished slot
	connect(this,SIGNAL(requestFinished(int,bool)),SLOT(finished(int,bool)));
	/// start the request and store the requestID
	requestID = get(path, to );
	/// block until the request is finished
	loop.exec();
	/// return the request status
	mutex.unlock();
	return status;
}

bool SyncHTTP::syncPost ( const QString & path, QIODevice * data, QIODevice * to)
{
	mutex.lock();
	///connect the requestFinished signal to our finished slot
	connect(this,SIGNAL(requestFinished(int,bool)),SLOT(finished(int,bool)));
	/// start the request and store the requestID
	requestID = post(path, data , to );
	/// block until the request is finished
	loop.exec();
	/// return the request status
	mutex.unlock();
	return status;
}

bool SyncHTTP::syncRequest ( const QHttpRequestHeader & header, QIODevice * data, QIODevice * to)
{
	mutex.lock();
	connect(this, SIGNAL(requestFinished(int,bool)), SLOT(finished(int,bool)));
	requestID = request(header, data, to);
	loop.exec();
	mutex.unlock();
	return status;
}

void SyncHTTP::finished(int idx, bool err)
{
	/// check to see if it's the request we made
	if(idx!=requestID)
		return;
	/// set status of the request
	status = !err;
	/// end the loop
	loop.exit();
}
