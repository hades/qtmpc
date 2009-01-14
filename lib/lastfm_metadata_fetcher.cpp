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

#include <QMap>
#include <QMessageBox>
#include <QImage>
#include <QString>
#include <QStringRef>
#include <QVariant>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QDate>
#include <QUrl>

#include "maiaXmlRpcClient.h"
#include "lastfm_metadata_fetcher.h"

LastFmMetadataFetcher::LastFmMetadataFetcher(QObject *parent)
	: QObject(parent),
		api_key("11172d35eb8cc2fd33250a9e45a2d486"),
		rpc(QUrl("http://ws.audioscrobbler.com/2.0/"))
{
	connect(&http, SIGNAL(requestFinished(int, bool)), this, SLOT(handleImageData(int, bool)));
}

void LastFmMetadataFetcher::setArtist(const QString &artist)
{
	_artist = artist;
}

void LastFmMetadataFetcher::setAlbumTitle(const QString &title)
{
	_title = title;
}

void LastFmMetadataFetcher::setTrackTitle(const QString &track)
{
	_track = track;
}

void LastFmMetadataFetcher::setFetchCover(bool fetch)
{
	_cover = fetch;
}

void LastFmMetadataFetcher::AlbumInfo()
{
	QVariantList args;
	QMap<QString, QVariant> mapped_args;

	mapped_args["artist"] = _artist;
	mapped_args["album"] = _title;
	mapped_args["api_key"] = api_key;

	args << mapped_args;

	rpc.call("album.getInfo", args,
	          this, SLOT(AlbumInfoDone(QVariant &)),
	          this, SLOT(AlbumInfoFault(int, const QString &)));
}

void LastFmMetadataFetcher::TrackInfo()
{

	QVariantList args;
	QMap<QString, QVariant> mapped_args;

	if (_artist == "" || _track == "")
		return;

	mapped_args["artist"] = _artist;
	mapped_args["track"] = _track;
	mapped_args["api_key"] = api_key;

	args << mapped_args;

	rpc.call("track.getInfo", args,
	          this, SLOT(TrackInfoDone(QVariant &)),
	          this, SLOT(TrackInfoFault(int, const QString &)));
}

void LastFmMetadataFetcher::AlbumInfoFault(int /*error*/, const QString &message)
{
	QString errorMessage(tr("Error while communicating with last.fm to retrieve an album cover.\n"));
	errorMessage.append("Error message is: ");
	errorMessage.append(message);

	QMessageBox::warning(
		NULL,
		tr("QtMPC: Warning"),
		errorMessage
	);

	if (_cover) {
		emit coverImage(QImage(), QString(), QString());
	}
}

void LastFmMetadataFetcher::TrackInfoFault(int /*error*/, const QString &message)
{
	QString errorMessage(tr("Error while communicating with last.fm to retrieve track info.\n"));
	errorMessage.append("Error message is: ");
	errorMessage.append(message);

	QMessageBox::warning(
		NULL,
		tr("QtMPC: Warning"),
		errorMessage
	);
}

void LastFmMetadataFetcher::AlbumInfoDone(QVariant &arg)
{
	QString xmldoc = arg.toString();
	xmldoc.replace("\\\"", "\"");
	QXmlStreamReader doc(xmldoc);
	bool albumFound = false;
	QDate date;

	while(!doc.atEnd()) {
		doc.readNext();

		if(doc.isStartElement() && doc.name() == "image") {
			if (doc.attributes().value("size").toString() == "extralarge") {
				QString url = doc.readElementText();
				if(url.isEmpty())
					continue;

				QStringList urlData = url.split('/', QString::SkipEmptyParts);

				http.setHost(urlData.at(1));
				urlData.removeFirst();
				urlData.removeFirst();
				int id = http.get("/" + urlData.join("/"));

				RequestInfo reqInfo;
				reqInfo.artist = _artist;
				reqInfo.album = _title;
				requestTypes.insert(id, reqInfo);

				albumFound = true;
			}
		} else if (doc.isStartElement() && doc.name() == "releasedate") {
			QString tmp = doc.readElementText().trimmed().split(",").at(0);
			date = QDate::fromString(tmp, "d MMM yyyy");
		}
	}

	if(!albumFound && _cover)
		emit coverImage(QImage(), QString(), QString());

	emit releaseDate(date);
}

/**
 * Get trackinfo.
 * TODO: Emit some signals
 */
void LastFmMetadataFetcher::TrackInfoDone(QVariant &arg)
{
	QString xmldoc = arg.toString();
	xmldoc.replace("\\\"", "\"");
	QXmlStreamReader doc(xmldoc);
	QDateTime date;

	while(!doc.atEnd()) {
		doc.readNext();

		if (doc.isStartElement() && doc.name() == "published") {
			QString tmp = doc.readElementText().trimmed().split(",").at(1).trimmed();
			tmp.chop(6);
			date = QDateTime::fromString(tmp, "d MMM yyyy hh:mm:ss");
		} else if (doc.isStartElement() && doc.name() == "content") {
			QString content = doc.readElementText();
			content = content.replace("&quot;", "\"");
			content = content.replace("\\'", "'");
		}
	}
}

void LastFmMetadataFetcher::handleImageData(int id, bool error)
{
	if(error) {
		qWarning("HTTP Errors...");
		return;
	}

	if (!requestTypes.contains(id))
		return;

	QImage img = QImage::fromData(http.readAll());
	RequestInfo reqInfo = requestTypes.take(id);
	emit coverImage(img, reqInfo.artist, reqInfo.album);
}
