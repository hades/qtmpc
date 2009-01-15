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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#ifdef ENABLE_KDE_SUPPORT
#include <KXmlGuiWindow>
#include <KStatusBar>
#else
#include <QMainWindow>
#endif

#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QHeaderView>
#include <QStringList>
#include <QDateTime>

#ifdef ENABLE_KDE_SUPPORT
#include "ui_main_window_kde.h"
#else
#include "ui_main_window.h"
#endif

#include "gui/musiclibrarymodel.h"
#include "gui/playlisttablemodel.h"
#include "gui/musiclibrarysortfiltermodel.h"
#include "gui/dirviewmodel.h"
#include "gui/dirviewproxymodel.h"
#include "lib/lastfm_metadata_fetcher.h"
#include "lib/lastfm_scrobbling.h"
#include "lib/mpdconnection.h"
#include "lib/mpddatabaseconnection.h"

class KAction;

class VolumeSliderEventHandler : public QObject
{
	Q_OBJECT

	public:
		VolumeSliderEventHandler(QSlider * const volSlider, QObject *parent = 0);

	protected:
		bool eventFilter(QObject *obj, QEvent *event);

	private:
		QSlider * const volSlider;
};

#ifdef ENABLE_KDE_SUPPORT
class MainWindow : public KXmlGuiWindow, private Ui::MainWindow
#else
class MainWindow : public QMainWindow, private Ui::MainWindow
#endif
{
	Q_OBJECT

	public:
		MainWindow(QWidget *parent = 0);

	protected:
		void closeEvent(QCloseEvent *event);

	private:
		MPDConnection mpd;
		MPDDatabaseConnection mpdDb;
		MPDStatus::State lastState;
		qint32 lastSongId;
		quint32 lastPlaylist;
		QDateTime lastDbUpdate;
		int fetchStatsFactor;
		int nowPlayingFactor;
		QTimer statusTimer;
		PlaylistTableModel playlistModel;
		dirViewModel dirviewModel;
		DirViewProxyModel dirProxyModel;
		MusicLibraryModel musicLibraryModel;
		MusicLibrarySortFilterModel libraryProxyModel;
		LastFmMetadataFetcher metadataFetcher;
		LastFmScrobbler scrobbler;
		bool draggingPositionSlider;
		const QIcon icon;
		QIcon playbackStop;
		QIcon playbackNext;
		QIcon playbackPrev;
		QIcon playbackPause;
		QIcon playbackPlay;
		QLabel bitrateLabel;
		QHeaderView *playlistTableViewHeader;
		VolumeSliderEventHandler *volumeSliderEventHandler;

		bool setupTrayIcon();
		void setupPlaylistViewMenu();
		void setupPlaylistViewHeader();
		QAction *playPauseAction;
		QAction *stopAction;
		QAction *nextAction;
		QAction *prevAction;
#ifdef ENABLE_KDE_SUPPORT
		KAction *quitAction;
#else
		QAction *quitAction;
#endif
		QAction *albumPlaylistViewAction;
		QAction *artistPlaylistViewAction;
		QAction *titlePlaylistViewAction;
		QAction *timePlaylistViewAction;
		QAction *trackPlaylistViewAction;
		QAction *discPlaylistViewAction;

		QSystemTrayIcon *trayIcon;
		QMenu *trayIconMenu;
		QString toolTipText;
		QMenu *playlistTableViewMenu;

		void addSelectionToPlaylist();
		void addDirViewSelectionToPlaylist();
		QStringList walkDirView(QModelIndex rootItem);

	private slots:
		int showPreferencesDialog(const int tab = 0);
#ifndef ENABLE_KDE_SUPPORT
		void showAboutDialog();
#endif
		void showStatisticsDialog();
		void setAlbumCover(QImage, QString, QString);
		void setAlbumReleaseDate(QDate);
		void positionSliderPressed();
		void positionSliderReleased();
		void nextTrack();
		void stopTrack();
		void playPauseTrack();
		void previousTrack();
		void setVolume(int vol);
		void setPosition();
		void setRandom(const int);
		void setRepeat(const int);
		void searchMusicLibrary();
		void updateCurrentSong(const Song *song);
		void updateStats();
		void updateStatus();
		void playlistItemActivated(const QModelIndex &);
		void removeFromPlaylist();
		void libraryItemActivated(const QModelIndex &);
		void addToPlaylistButtonActivated();
		void dirViewAddToPlaylistPushButtonActivated();
		void trayIconClicked(QSystemTrayIcon::ActivationReason reason);
		void crossfadingChanged(const int seconds);
		void playlistTableViewContextMenuClicked();
		void updateIntervalChanged(const int mseconds);

		void saveSplitterState(int, int);

		void playListTableViewToggleAlbum(const bool visible);
		void playListTableViewToggleArtist(const bool visible);
		void playListTableViewToggleTrack(const bool visible);
		void playListTableViewToggleTime(const bool visible);
		void playListTableViewToggleTitle(const bool visible);
		void playListTableViewToggleDisc(const bool visible);
		void playListTableViewSaveState();

		void updatePlayListStatus();

		void movePlaylistItems(const QList<quint32> items, const quint32 diff, const int max);
		void addFilenamesToPlaylist(const QStringList filenames, const int row, const int size);

	signals:
		void submitSong();
		void startSong();
		void stopSong();
		void pauseSong();
		void resumeSong();
		void nowPlaying();
};

#endif
