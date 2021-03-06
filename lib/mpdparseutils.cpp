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

#include <QList>
#include <QString>
#include <QStringList>

#include "gui/dirviewitemroot.h"
#include "gui/dirviewitemdir.h"
#include "gui/dirviewitemfile.h"
#include "gui/musiclibraryitemartist.h"
#include "gui/musiclibraryitemalbum.h"
#include "gui/musiclibraryitemsong.h"
#include "mpdparseutils.h"
#include "mpdstats.h"
#include "mpdstatus.h"
#include "song.h"


void MPDParseUtils::parseStats(const QByteArray * const data)
{
	MPDStats * const stats = MPDStats::getInstance();
	stats->acquireWriteLock();

	QList<QByteArray> lines = data->split('\n');
	QList<QByteArray> tokens;

	int amountOfLines = lines.size();

	for(int i = 0; i < amountOfLines; i++) {
		tokens = lines.at(i).split(':');

		if(tokens.at(0) == "artists") {
			stats->setArtists(tokens.at(1).toUInt());
		} else if(tokens.at(0) == "albums") {
			stats->setAlbums(tokens.at(1).toUInt());
		} else if(tokens.at(0) == "songs") {
			stats->setSongs(tokens.at(1).toUInt());
		} else if(tokens.at(0) == "uptime") {
			stats->setUptime(tokens.at(1).toUInt());
		} else if(tokens.at(0) == "playtime") {
			stats->setPlaytime(tokens.at(1).toUInt());
		} else if(tokens.at(0) == "db_playtime") {
			stats->setDbPlaytime(tokens.at(1).toUInt());
		} else if(tokens.at(0) == "db_update") {
			stats->setDbUpdate(tokens.at(1).toUInt());
		}
	}

	stats->releaseWriteLock();
}

void MPDParseUtils::parseStatus(const QByteArray * const data)
{
	MPDStatus * const status = MPDStatus::getInstance();
	QList<QByteArray> lines = data->split('\n');
	QList<QByteArray> tokens;

	status->acquireWriteLock();

	int amountOfLines = lines.size();

	for(int i = 0; i < amountOfLines; i++) {
		tokens = lines.at(i).split(':');

		if(tokens.at(0) == "volume") {
			status->setVolume(tokens.at(1).toUInt());
		} else if(tokens.at(0) == "repeat") {
			if(tokens.at(1).trimmed() == "1") {
				status->setRepeat(true);
			} else {
				status->setRepeat(false);
			}
		} else if(tokens.at(0) == "random") {
			if(tokens.at(1).trimmed() == "1") {
				status->setRandom(true);
			} else {
				status->setRandom(false);
			}
		} else if(tokens.at(0) == "playlist") {
			status->setPlaylist(tokens.at(1).toUInt());
		} else if(tokens.at(0) == "playlistlength") {
			status->setPlaylistLength(tokens.at(1).toInt());
		} else if(tokens.at(0) == "playlistqueue") {
			status->setPlaylistQueue(tokens.at(1).toInt());
		} else if(tokens.at(0) == "xfade") {
			status->setXfade(tokens.at(1).toInt());
		} else if(tokens.at(0) == "state") {
			if(tokens.at(1).contains("play")) {
				status->setState(MPDStatus::State_Playing);
			} else if(tokens.at(1).contains("stop")) {
				status->setState(MPDStatus::State_Stopped);
			} else {
				status->setState(MPDStatus::State_Paused);
			}
		} else if(tokens.at(0) == "song") {
			status->setSong(tokens.at(1).toInt());
		} else if(tokens.at(0) == "songid") {
			status->setSongId(tokens.at(1).toInt());
		} else if(tokens.at(0) == "time") {
			status->setTimeElapsed(tokens.at(1).toInt());
			status->setTimeTotal(tokens.at(2).toInt());
		} else if(tokens.at(0) == "bitrate") {
			status->setBitrate(tokens.at(1).toUInt());
		} else if(tokens.at(0) == "audio") {
		} else if(tokens.at(0) == "updating_db") {
			status->setUpdatingDb(tokens.at(1).toInt());
		} else if(tokens.at(0) == "error") {
			status->setError(tokens.at(1));
		}
	}

	status->releaseWriteLock();
}

Song * MPDParseUtils::parseSong(const QByteArray * const data)
{
	Song *song = new Song;

	QString tmpData = QString::fromUtf8(data->constData());
	QStringList lines = tmpData.split('\n');
	QStringList tokens;
	QString element;
	QString value;

	int amountOfLines = lines.size();

	for(int i = 0; i < amountOfLines; i++) {
		tokens = lines.at(i).split(':');
		element = tokens.takeFirst();
		value = tokens.join(":");
		value = value.trimmed();

		if(element == "file") {
			song->file = value;
			song->file.replace("\"", "\\\"");
		} else if(element == "Time") {
			song->time = value.toUInt();
		} else if(element == "Album") {
			song->album = value;
		} else if(element == "Artist") {
			song->artist = value;
		} else if(element == "Title") {
			song->title = value;
		} else if(element == "Track") {
			song->track = value.toInt();
		} else if(element == "Pos") {
			song->pos = value.toInt();
		} else if(element == "Id") {
			song->id = value.toUInt();
		} else if (element == "Disc") {
			song->disc = value.toUInt();
		}
	}

	return song;
}

QList<Song *> * MPDParseUtils::parseSongs(const QByteArray * const data)
{
	QList<Song *> *songs = new QList<Song *>;
	QByteArray song;

	QList<QByteArray> lines = data->split('\n');

	int amountOfLines = lines.size();

	for(int i = 0; i < amountOfLines; i++) {
		song += lines.at(i);
		song += "\n";
		if(i == lines.size() - 1 || lines.at(i + 1).startsWith("file:")) {
			songs->append(parseSong(&song));
			song.clear();
		}
	}

	return songs;
}

QList<MusicLibraryItemArtist *> * MPDParseUtils::parseLibraryItems(const QByteArray * const data)
{
	QList<MusicLibraryItemArtist *> *artists = new QList<MusicLibraryItemArtist *>;
	QByteArray currentItem;
	MusicLibraryItemArtist *artistItem = NULL;
	MusicLibraryItemAlbum *albumItem = NULL;
	MusicLibraryItemSong *songItem = NULL;
	Song *currentSong;
	bool found = false;

	QList<QByteArray> lines = data->split('\n');

	int amountOfLines = lines.size();

	for(int i = 0; i < amountOfLines; i++) {
		currentItem += lines.at(i);
		currentItem += "\n";
		if(i == lines.size() - 1 || lines.at(i + 1).startsWith("file:")) {
			currentSong = parseSong(&currentItem);
			currentItem.clear();

			if(currentSong->isEmpty()) {
				delete currentSong;
				continue;
			}

			currentSong->fillEmptyFields();

			int amountOfArtists = artists->size();

			// Check if artist already exists
			for(int i = 0; i < amountOfArtists; i++) {
				if(artists->at(i)->data(0) == currentSong->artist) {
					artistItem = artists->at(i);
					found = true;
				}
			}

			if(!found) {
				artistItem = new MusicLibraryItemArtist(currentSong->artist);
				artists->append(artistItem);
			}

			found = false;

			int amountOfAlbums = artistItem->childCount();

			// Check if album already exists
			for(int i = 0; i < amountOfAlbums; i++) {
				if(artistItem->child(i)->data(0) == currentSong->album) {
					albumItem = static_cast<MusicLibraryItemAlbum *>(artistItem->child(i));
					found = true;
				}
			}

			if(!found) {
				albumItem = new MusicLibraryItemAlbum(currentSong->album, artistItem);
				artistItem->appendChild(albumItem);
			}

			found = false;

			// Add song to album (possibly in track order)
			songItem = new MusicLibraryItemSong(currentSong->title, albumItem);
			songItem->setFile(currentSong->file);
			songItem->setTrack(currentSong->track);
			songItem->setDisc(currentSong->disc);
			albumItem->appendChild(songItem);

			delete currentSong;
		}
	}

	return artists;
}

DirViewItemRoot * MPDParseUtils::parseDirViewItems(const QByteArray * const data)
{
	QList<QByteArray> lines = data->split('\n');

	DirViewItemRoot * rootItem = new DirViewItemRoot("");
	DirViewItem * currentDir = rootItem;
	QStringList currentDirList;

	int amountOfLines = lines.size();
	for (int i = 0; i < amountOfLines; i++) {
		QString line(lines.at(i));

		if (line.startsWith("file: ")) {
			line.remove(0, 6);
			QStringList parts = line.split("/");

			if (currentDir->type() == DirViewItem::Type_Root)
				static_cast<DirViewItemRoot *>(currentDir)->insertFile(parts.at(parts.size()-1));
			else
				static_cast<DirViewItemDir *>(currentDir)->insertFile(parts.at(parts.size()-1));
		} else if (line.startsWith("directory: ")) {
			line.remove(0, 11);
			QStringList parts = line.split("/");

			/* Check how much matches */
			int depth = 0;
			for (int j = 0; j < currentDirList.size() && j < parts.size(); j++) {
				if (currentDirList.at(j) != parts.at(j))
					break;
				depth++;
			}

			for (int j = currentDirList.size(); j > depth; j--) {
				currentDir = currentDir->parent();
			}

			if (currentDir->type() == DirViewItem::Type_Root)
				currentDir = static_cast<DirViewItemRoot *>(currentDir)->createDirectory(parts.at(parts.size()-1));
			else
				currentDir = static_cast<DirViewItemDir *>(currentDir)->createDirectory(parts.at(parts.size()-1));

			currentDirList = parts;
		}
	}

	return rootItem;
}

/**
 * Convert a number of seconds to a readable time format
 * d days hh:mm:ss
 *
 * @param totalseconds Total number of seconds to convert
 * @return A fromatted string
 */
QString MPDParseUtils::seconds2formattedString(const quint32 totalseconds)
{
	QString string;

	//Get the days,hours,minutes and seconds out of the total seconds
	quint32 days = totalseconds / 86400;
	quint32 rest = totalseconds - (days * 86400);
	quint32 hours = rest / 3600;
	rest = rest - (hours * 3600);
	quint32 minutes = rest / 60;
	quint32 seconds = rest - (minutes * 60);

	//Convert hour,minutes and seconds to a QTime for easier parsing
	QTime time(hours, minutes, seconds);

	if (days == 1) {
		string.append(QString::number(days) + " day ");
	} else if (days > 1) {
		string.append(QString::number(days) + " days ");
	}

	string.append(time.toString("hh:mm:ss"));

	return string;
}
