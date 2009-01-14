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

#ifndef LASTFM_METADATA_FETCHER_H
#define LASTFM_METADATA_FETCHER_H

#include "maiaXmlRpcClient.h"

#include <QHttp>
#include <QImage>
#include <QString>
#include <QMap>

class LastFmMetadataFetcher : public QObject
{
	Q_OBJECT

	public:
		LastFmMetadataFetcher(QObject *parent = 0);
		void setArtist(const QString &artist);
		void setAlbumTitle(const QString &title);
		void setFetchCover(bool fetch);
		void setTrackTitle(const QString &track);
		void AlbumInfo();
		void TrackInfo();

	private:
		typedef struct
		{
			QString artist;
			QString album;
		} RequestInfo;

		const QString api_key;
		MaiaXmlRpcClient rpc;

		QHttp http;
		QString request;
		QString _artist;
		QString _title;
		QString _track;
		bool _cover;
		QMap<int, RequestInfo> requestTypes;

	private slots:
		void handleImageData(int id, bool error);
		void AlbumInfoFault(int error, const QString &message);
		void AlbumInfoDone(QVariant &arg);
		void TrackInfoFault(int error, const QString &message);
		void TrackInfoDone(QVariant &arg);

	signals:
		void coverImage(QImage, QString, QString);
		void releaseDate(QDate);
};

#endif
