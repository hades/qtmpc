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
#ifndef SYNCHTTP_H
#define SYNCHTTP_H

#include <QHttp>
#include <QEventLoop>
#include <QBuffer>
#include <QMutex>

/**
* Provide a synchronous api over QHttp
* Uses a QEventLoop to block until the request is completed
* @author Iulian M <eti@erata.net>
*/
class SyncHTTP: public QHttp
{
	Q_OBJECT

	public:
		SyncHTTP( QObject * parent = 0 );
		SyncHTTP( const QString & hostName, quint16 port = 80, QObject * parent = 0 );
		
		virtual ~SyncHTTP(){}
		
		bool syncGet ( const QString & path, QIODevice * to = 0 );		
		bool syncPost ( const QString & path, QIODevice * data, QIODevice * to = 0 );
		bool syncRequest ( const QHttpRequestHeader & header, QIODevice * data = 0, QIODevice * to = 0);
		
	protected slots:
		virtual void finished(int idx, bool err);
		
	private:
		int requestID;
		bool status;
		QEventLoop loop;
		QMutex mutex;
};

#endif
