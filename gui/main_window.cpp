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

#include <QtGui>
#include <QIcon>
#include <QPixmap>
#include <QSet>
#include <QSettings>
#include <QString>
#include <QKeyEvent>
#include <QHeaderView>
#include <cstdlib>

#ifdef ENABLE_KDE_SUPPORT
#include <KApplication>
#include <KAction>
#include <KIcon>
#include <KLocale>
#include <KActionCollection>
#include <KPassivePopup>
#include <KStandardAction>
#include <kaboutapplicationdialog.h>
#endif

#include "main_window.h"
#include "musiclibraryitemsong.h"
#include "preferences_dialog.h"
#include "about_dialog.h"
#include "statistics_dialog.h"
#include "lib/mpdstats.h"
#include "lib/mpdstatus.h"
#include "mpdparseutils.h"
#include "lib/lastfm_scrobbling.h"


VolumeSliderEventHandler::VolumeSliderEventHandler(QSlider * const volSlider, QObject *parent)
		: QObject(parent),
		  volSlider(volSlider)
{
}

bool VolumeSliderEventHandler::eventFilter(QObject *obj, QEvent *event)
{
	if(event->type() == QEvent::Wheel) {
		int numDegrees = static_cast<QWheelEvent *>(event)->delta() / 8;
		int numSteps = numDegrees / 15;
		volSlider->setValue(volSlider->value() + numSteps);
		return true;
	}

	return QObject::eventFilter(obj, event);
}

#ifdef ENABLE_KDE_SUPPORT
MainWindow::MainWindow(QWidget *parent) : KXmlGuiWindow(parent),
#else
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
#endif
		lastState(MPDStatus::State_Inactive),
		lastSongId(-1),
		fetchStatsFactor(0),
		draggingPositionSlider(false),
		icon(QIcon(":/images/icon.svg"))
{
#ifdef ENABLE_KDE_SUPPORT
	QWidget *widget = new QWidget(this);
	setupUi(widget);
	setCentralWidget(widget);

	KStandardAction::quit(kapp, SLOT(quit()), actionCollection());
	KStandardAction::preferences(this, SLOT(showPreferencesDialog()), actionCollection());

	KAction *action_Update_database = new KAction(this);
	action_Update_database->setText(i18n("Update database"));
	actionCollection()->addAction("updatedatabase", action_Update_database);

	KAction *action_Show_statistics = new KAction(this);
	action_Show_statistics->setText(i18n("Show statistics"));
	actionCollection()->addAction("showstatistics", action_Show_statistics);

	setupGUI();
#else
	setupUi(this);
#endif
	QSettings settings;

	setVisible(true);

	// Setup event handler for volume adjustment
	volumeSliderEventHandler = new VolumeSliderEventHandler(volumeSlider, this);

// KDE does this for us
#ifndef ENABLE_KDE_SUPPORT
	this->setWindowIcon(icon);
#endif

#ifdef ENABLE_KDE_SUPPORT
	playbackStop = KIcon("media-playback-stop");
	playbackNext = KIcon("media-skip-forward");
	playbackPrev = KIcon("media-skip-backward");
	playbackPlay = KIcon("media-playback-start");
	playbackPause = KIcon("media-playback-pause");

	addToPlaylistPushButton->setIcon(KIcon("list-add"));
	dirViewAddToPlaylistPushButton->setIcon(KIcon("list-add"));

	removeAllFromPlaylistPushButton->setIcon(KIcon("edit-clear-list"));
	removeFromPlaylistPushButton->setIcon(KIcon("list-remove"));

	searchPushButton->setIcon(KIcon("edit-find"));
#elif QT_VERSION >= 0x040400
	QCommonStyle style;
	playbackStop = style.standardIcon(QStyle::SP_MediaStop);
	playbackNext = style.standardIcon(QStyle::SP_MediaSkipForward);
	playbackPrev = style.standardIcon(QStyle::SP_MediaSkipBackward);
	playbackPlay = style.standardIcon(QStyle::SP_MediaPlay);
	playbackPause = style.standardIcon(QStyle::SP_MediaPause);
#else
	playbackStop = QIcon(":/images/player_stop.svg");
	playbackNext = QIcon(":/images/player_next.svg");
	playbackPrev = QIcon(":/images/player_prev.svg");
	playbackPlay = QIcon(":/images/player_play.svg");
	playbackPause = QIcon(":/images/player_pause.svg");
#endif

	stopTrackButton->setIcon(playbackStop);
	nextTrackButton->setIcon(playbackNext);
	prevTrackButton->setIcon(playbackPrev);

	// Tray stuf
	if (setupTrayIcon() && settings.value("systemtray").toBool())
		trayIcon->show();

	//PlaylistView
	setupPlaylistViewHeader();
	setupPlaylistViewMenu();

	// Status bar
	bitrateLabel.setText("Bitrate: ");
	statusBar()->addWidget(&bitrateLabel);

	/* Start scrobbler thread
	 */
	scrobbler.start();

	// Start connection threads
	mpd.start();
	mpdDb.start();

	// Set connection data
	mpd.setHostname(settings.value("connection/host").toString());
	mpd.setPort(settings.value("connection/port").toInt());
	mpdDb.setHostname(settings.value("connection/host").toString());
	mpdDb.setPort(settings.value("connection/port").toInt());

	while(!mpd.connectToMPD()) {
		if(!showPreferencesDialog())
			exit(EXIT_FAILURE);

		mpd.setHostname(settings.value("connection/host").toString());
		mpd.setPort(settings.value("connection/port").toInt());
		mpdDb.setHostname(settings.value("connection/host").toString());
		mpdDb.setPort(settings.value("connection/port").toInt());

		qWarning("Retrying");
	}

	volumeSlider->installEventFilter(volumeSliderEventHandler);

	libraryProxyModel.setSourceModel(&musicLibraryModel);
	libraryTreeView->setModel(&libraryProxyModel);
	libraryTreeView->sortByColumn(0, Qt::AscendingOrder);
	libraryTreeView->setDragEnabled(true);

	dirProxyModel.setSourceModel(&dirviewModel);
	dirTreeView->setModel(&dirProxyModel);
	dirTreeView->sortByColumn(0, Qt::AscendingOrder);

	// Playlist
	connect(&playlistModel, SIGNAL(filesAddedInPlaylist(const QStringList, const int, const int)), this,
	        SLOT(addFilenamesToPlaylist(const QStringList, const int, const int)));
	connect(&playlistModel, SIGNAL(moveInPlaylist(const QList<quint32>, const quint32, const int)), this,
	        SLOT(movePlaylistItems(const QList<quint32>, const quint32, const int)));
	connect(playlistTableView, SIGNAL(pressed(const QModelIndex)), &playlistModel, SLOT(clicked(const QModelIndex)));

	// Playlist table view
	playlistTableView->setModel(&playlistModel);
	playlistTableView->verticalHeader()->hide();
	playlistTableView->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
	playlistTableView->setDragEnabled(true);
	playlistTableView->setAcceptDrops(true);
	playlistTableView->setDropIndicatorShown(true);

	// MPD
	connect(&mpd, SIGNAL(statsUpdated()), this, SLOT(updateStats()));
	connect(&mpd, SIGNAL(statusUpdated()), this, SLOT(updateStatus()), Qt::DirectConnection);
	connect(&mpd, SIGNAL(playlistUpdated(QList<Song *> *)), &playlistModel, SLOT(updateSongs(QList<Song *> *)));
	connect(&mpd, SIGNAL(currentSongUpdated(const Song *)), this, SLOT(updateCurrentSong(const Song *)));
	connect(&mpdDb, SIGNAL(musicLibraryUpdated(QList<MusicLibraryItemArtist *> *, QDateTime)), &musicLibraryModel, SLOT(updateLibrary(QList<MusicLibraryItemArtist *> *, QDateTime)));
	connect(&mpdDb, SIGNAL(dirViewUpdated(DirViewItemRoot *)), &dirviewModel, SLOT(dirViewUpdated(DirViewItemRoot *)));

	// Metadata
	connect(&metadataFetcher, SIGNAL(coverImage(QImage, QString, QString)), this, SLOT(setAlbumCover(QImage, QString, QString)));
	connect(&metadataFetcher, SIGNAL(releaseDate(QDate)), this, SLOT(setAlbumReleaseDate(QDate)));

	// GUI
#ifndef ENABLE_KDE_SUPPORT
	connect(action_Quit, SIGNAL(triggered()), qApp, SLOT(quit()));
	connect(action_Preferences, SIGNAL(triggered(bool)), this, SLOT(showPreferencesDialog()));
	connect(action_About, SIGNAL(triggered(bool)), this, SLOT(showAboutDialog()));
#endif
	connect(action_Update_database, SIGNAL(triggered(bool)), &mpdDb, SLOT(update()));
	connect(action_Show_statistics, SIGNAL(triggered(bool)), this, SLOT(showStatisticsDialog()));
	connect(splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(saveSplitterState(int, int)));
	connect(volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(setVolume(int)));
	connect(positionSlider, SIGNAL(sliderPressed()), this, SLOT(positionSliderPressed()));
	connect(positionSlider, SIGNAL(sliderReleased()), this, SLOT(setPosition()));
	connect(positionSlider, SIGNAL(sliderReleased()), this, SLOT(positionSliderReleased()));
	connect(prevTrackButton, SIGNAL(clicked(bool)), this, SLOT(previousTrack()));
	connect(stopTrackButton, SIGNAL(clicked(bool)), this, SLOT(stopTrack()));
	connect(playPauseTrackButton, SIGNAL(clicked(bool)), this, SLOT(playPauseTrack()));
	connect(nextTrackButton, SIGNAL(clicked(bool)), this, SLOT(nextTrack()));
	connect(randomCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setRandom(int)));
	connect(repeatCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setRepeat(int)));
	connect(searchPushButton, SIGNAL(clicked(bool)), this, SLOT(searchMusicLibrary()));
	connect(searchFieldComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(searchMusicLibrary()));
	connect(searchTextLineEdit, SIGNAL(returnPressed()), this, SLOT(searchMusicLibrary()));
	connect(searchTextLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(searchMusicLibrary()));
	connect(playlistTableView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(playlistItemActivated(const QModelIndex &)));
	connect(removeAllFromPlaylistPushButton, SIGNAL(clicked(bool)), &mpd, SLOT(clear()));
	connect(removeFromPlaylistPushButton, SIGNAL(clicked(bool)), this, SLOT(removeFromPlaylist()));
	connect(libraryTreeView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(libraryItemActivated(const QModelIndex &)));
	connect(addToPlaylistPushButton, SIGNAL(clicked(bool)), this, SLOT(addToPlaylistButtonActivated()));
	connect(dirViewAddToPlaylistPushButton, SIGNAL(clicked(bool)), this, SLOT(dirViewAddToPlaylistPushButtonActivated()));
	connect(&playlistModel, SIGNAL(playListStatsUpdated()), this, SLOT(updatePlayListStatus()));

	// Timer
	connect(&statusTimer, SIGNAL(timeout()), &mpd, SLOT(getStatus()));

	//Scrobbler
	connect(&scrobbler, SIGNAL(authFailed(const int)), this, SLOT(showPreferencesDialog(const int)));
	connect(this, SIGNAL(stopSong()), &scrobbler, SLOT(stopScrobblerTimer()));
	connect(this, SIGNAL(startSong()), &scrobbler, SLOT(startScrobbleTimer()));
	connect(this, SIGNAL(pauseSong()), &scrobbler, SLOT(pauseScrobblerTimer()));
	connect(this, SIGNAL(resumeSong()), &scrobbler, SLOT(resumeScrobblerTimer()));
	connect(this, SIGNAL(nowPlaying(QString, QString, QString, quint32, quint32)),
					 &scrobbler, SLOT(nowPlaying(QString, QString, QString, quint32, quint32)));

	splitter->restoreState(settings.value("splitterSizes").toByteArray());

	mpd.getStatus();
	mpd.getStats();

	/* Check if we need to get a new db or not */
	if (!musicLibraryModel.fromXML(MPDStats::getInstance()->dbUpdate()))
		mpdDb.listAllInfo(MPDStats::getInstance()->dbUpdate());

	statusTimer.start(settings.value("connection/interval", 1000).toInt());
	mpdDb.listAll();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (trayIcon != NULL && trayIcon->isVisible())
		hide();
	else
		qApp->quit();

	event->ignore();
}

int MainWindow::showPreferencesDialog(const int tab)
{
	PreferencesDialog pref(tab, this);
	if (mpd.isConnected()) {
		if (trayIcon != NULL)
			connect(&pref, SIGNAL(systemTraySet(bool)), trayIcon, SLOT(setVisible(bool)));
		connect(&pref, SIGNAL(crossfadingChanged(const int)), this, SLOT(crossfadingChanged(const int)));
		connect(&pref, SIGNAL(updateIntervalChanged(const int)), this, SLOT(updateIntervalChanged(const int)));
	}
	connect(&pref, SIGNAL(lastFmReAuth(bool)), &scrobbler, SLOT(clearAuth(bool)), Qt::QueuedConnection);
	return pref.exec();
}

#ifndef ENABLE_KDE_SUPPORT
void MainWindow::showAboutDialog()
{
	AboutDialog about(this);
	about.exec();
}
#endif

void MainWindow::showStatisticsDialog()
{
	StatisticsDialog stats(this);
	stats.exec();
}

void MainWindow::setAlbumCover(QImage img, QString artist, QString album)
{
	if(img.isNull()) {
		albumCoverLabel->setText("No cover available");
		return;
	}

	// Display image
	QPixmap pixmap = QPixmap::fromImage(img);
	pixmap = pixmap.scaled(QSize(150, 150), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	albumCoverLabel->setPixmap(pixmap);

	// Save image to avoid downloading it next time
	QDir dir(QDir::home());
	if(!dir.exists(".QtMPC")) {
		if(!dir.mkdir(".QtMPC")) {
			qWarning("Couldn't create directory for storing album covers");
			return;
		}
	}

	dir.cd(".QtMPC");
	QString file(QFile::encodeName(artist + " - " + album + ".jpg"));
	img.save(dir.absolutePath() + QDir::separator() + file);
}

void MainWindow::setAlbumReleaseDate(QDate date)
{
	if (date.isValid())
		releaseDateLabel->setText(date.toString("d MMMM yyyy"));
}

void MainWindow::positionSliderPressed()
{
	draggingPositionSlider = true;
}

void MainWindow::positionSliderReleased()
{
	draggingPositionSlider = false;
}

void MainWindow::nextTrack()
{
	mpd.goToNext();
	mpd.getStatus();
}

void MainWindow::stopTrack()
{
	mpd.stopPlaying();
	mpd.getStatus();

	stopAction->setEnabled(false);
	playPauseAction->setText("&Play");
}

void MainWindow::playPauseTrack()
{
	MPDStatus * const status = MPDStatus::getInstance();

	if(status->state() == MPDStatus::State_Playing)
		mpd.setPause(true);
	else if(status->state() == MPDStatus::State_Paused)
		mpd.setPause(false);
	else
		mpd.startPlayingSong();

	mpd.getStatus();
}

void MainWindow::previousTrack()
{
	mpd.goToPrevious();
	mpd.getStatus();
}

void MainWindow::setPosition()
{
	mpd.setSeekId(MPDStatus::getInstance()->songId(), positionSlider->value());
}

void MainWindow::setVolume(int vol)
{
	mpd.setVolume(vol);
}

void MainWindow::setRandom(const int state)
{
	if(state == Qt::Checked)
		mpd.setRandom(true);
	else
		mpd.setRandom(false);
}

void MainWindow::setRepeat(const int state)
{
	if(state == Qt::Checked)
		mpd.setRepeat(true);
	else
		mpd.setRepeat(false);
}

void MainWindow::searchMusicLibrary()
{
	if(searchTextLineEdit->text().isEmpty()) {
		libraryProxyModel.setFilterRegExp("");
		return;
	}

	libraryProxyModel.setFilterField(searchFieldComboBox->currentIndex());
	libraryProxyModel.setFilterRegExp(searchTextLineEdit->text());
}

void MainWindow::updateCurrentSong(const Song *song)
{
	QSettings settings;

	// Determine if album cover should be updated
	if(trackArtistLabel->text() != song->artist || trackAlbumLabel->text() != song->album) {
		// Check if cover is already cached
		QString dir(QDir::toNativeSeparators(QDir::homePath()) + QDir::separator() + ".QtMPC" + QDir::separator());
		QString file(QFile::encodeName(song->artist + " - " + song->album + ".jpg"));
		metadataFetcher.setArtist(song->artist);
		metadataFetcher.setAlbumTitle(song->album);
		if(QFile::exists(dir + file)) {
			// Display image
			QPixmap pixmap = QPixmap::fromImage(QImage(dir + file));
			pixmap = pixmap.scaled(QSize(150, 150), Qt::KeepAspectRatio, Qt::SmoothTransformation);
			albumCoverLabel->setPixmap(pixmap);
			metadataFetcher.setFetchCover(true);
		} else {
			// Fetch image
			metadataFetcher.setFetchCover(true);
		}
		releaseDateLabel->setText("");
		metadataFetcher.AlbumInfo();
	}

	metadataFetcher.setTrackTitle(song->title);
	metadataFetcher.TrackInfo();

	trackTitleLabel->setText(song->title);
	trackArtistLabel->setText(song->artist);
	trackAlbumLabel->setText(song->album);

	playlistModel.updateCurrentSong(song->id);

	if (settings.value("systemtrayPopup").toBool() && trayIcon != NULL && trayIcon->isVisible() && isHidden()) {
		if (!song->title.isEmpty() && !song->artist.isEmpty() && !song->album.isEmpty()) {
			QString text = "album:  " + song->album + "\n";
			if (song->track > 0)
				text += "track:  " + QString::number(song->track) + "\n";
			text += "length: " + Song::formattedTime(song->time);
#ifdef ENABLE_KDE_SUPPORT
			KPassivePopup::message(song->artist + " - " + song->title, text, trayIcon);
#else
			trayIcon->showMessage(song->artist + " - " + song->title, text,
				QSystemTrayIcon::Information, 5000);
#endif
		}
	}

	if (song->track > 0)
		toolTipText = QString::number(song->track) + " - " + song->artist + " - " + song->title + "\n";
	else
		toolTipText = song->artist + " - " + song->title + "\n";
	toolTipText += "album:  " + song->album + "\n";

	if (settings.value("lastfm/enabled", "false").toBool()) {
		emit startSong();
		emit nowPlaying(song->artist, song->album, song->title, song->track, song->time);
	}

	delete song;
}

void MainWindow::updateStats()
{
	MPDStats * const stats = MPDStats::getInstance();

	/*
	 * Check if remote db is more recent than local one
	 * Also update the dirview
	 */
	if(lastDbUpdate.isValid() && stats->dbUpdate() > lastDbUpdate) {
		mpdDb.listAllInfo(stats->dbUpdate());
		mpdDb.listAll();
	}

	lastDbUpdate = stats->dbUpdate();
}

void MainWindow::updateStatus()
{
	MPDStatus * const status = MPDStatus::getInstance();
	QString timeElapsedFormattedString;
	QSettings settings;

	// Retrieve stats every 5 seconds
	fetchStatsFactor = (fetchStatsFactor + 1) % 5;
	if(fetchStatsFactor == 0)
		mpd.getStats();

	if(!draggingPositionSlider) {
		if(status->state() == MPDStatus::State_Stopped
				|| status->state() == MPDStatus::State_Inactive) {
			positionSlider->setValue(0);
		} else {
			positionSlider->setMaximum(status->timeTotal());
			positionSlider->setValue(status->timeElapsed());
		}
	}

	volumeSlider->setValue(status->volume());

	if(status->random())
		randomCheckBox->setCheckState(Qt::Checked);
	else
		randomCheckBox->setCheckState(Qt::Unchecked);

	if(status->repeat())
		repeatCheckBox->setCheckState(Qt::Checked);
	else
		repeatCheckBox->setCheckState(Qt::Unchecked);

	if(status->state() == MPDStatus::State_Stopped
			|| status->state() == MPDStatus::State_Inactive) {
		timeElapsedFormattedString = "00:00 / 00:00";
	} else {
		timeElapsedFormattedString += Song::formattedTime(status->timeElapsed());
		timeElapsedFormattedString += " / ";
		timeElapsedFormattedString += Song::formattedTime(status->timeTotal());
	}

	songTimeElapsedLabel->setText(timeElapsedFormattedString);

	switch(status->state()) {
		case MPDStatus::State_Playing:
			// Main window
			playPauseTrackButton->setIcon(playbackPause);
			playPauseTrackButton->setEnabled(true);
			playPauseTrackButton->setChecked(false);
			stopTrackButton->setEnabled(true);

			// Tray icon
			playPauseAction->setText("&Pause");
			playPauseAction->setIcon(playbackPause);
			stopAction->setEnabled(true);

			break;
		case MPDStatus::State_Inactive:
		case MPDStatus::State_Stopped:
			// Main window
			playPauseTrackButton->setIcon(playbackPlay);
			playPauseTrackButton->setEnabled(true);
			stopTrackButton->setEnabled(false);
			trackTitleLabel->setText("");
			trackArtistLabel->setText("");
			trackAlbumLabel->setText("");
			albumCoverLabel->setPixmap(QPixmap());

			// Tray icon
			stopAction->setEnabled(false);
			playPauseAction->setText("&Play");
			playPauseAction->setIcon(playbackPlay);

			break;
		case MPDStatus::State_Paused:
			// Main window
			playPauseTrackButton->setIcon(playbackPlay);
			playPauseTrackButton->setEnabled(true);
			playPauseTrackButton->setChecked(true);
			stopTrackButton->setEnabled(true);

			// Tray icon
			playPauseAction->setText("&Play");
			playPauseAction->setIcon(playbackPlay);
			stopAction->setEnabled(true);
			break;
		default:
			qDebug("Invalid state");
			break;
	}

	// Check if song has changed or we're playing again after being stopped
	// and update song info if needed
	if(lastState == MPDStatus::State_Inactive
			|| (lastState == MPDStatus::State_Stopped
				&& status->state() == MPDStatus::State_Playing)
			|| lastSongId != status->songId())
		mpd.currentSong();

	/* Update scrobbler stuff */
	if (settings.value("lastfm/enabled", "false").toBool()) {
		if (lastState == MPDStatus::State_Playing &&
			status->state() == MPDStatus::State_Paused)
			emit pauseSong();
		else if (lastState == MPDStatus::State_Paused &&
			status->state() == MPDStatus::State_Playing)
			emit resumeSong();
		else if (lastState != MPDStatus::State_Stopped &&
			status->state() == MPDStatus::State_Stopped){
			emit stopSong();
			//emit submitSong();
		}
	}

	// Set TrayIcon tooltip
	QString text = toolTipText;
	text += "time: " + timeElapsedFormattedString;
	if (trayIcon != NULL)
		trayIcon->setToolTip(text);

	// Check if playlist has changed and update if needed
	if(lastState == MPDStatus::State_Inactive
			|| lastPlaylist < status->playlist()) {
		mpd.playListInfo();
	}

	// Display bitrate
	bitrateLabel.setText("Bitrate: " + QString::number(status->bitrate()));

	// Update status info
	lastState = status->state();
	lastSongId = status->songId();
	lastPlaylist = status->playlist();
}

void MainWindow::playlistItemActivated(const QModelIndex &index)
{
	mpd.startPlayingSongId(playlistModel.getIdByRow(index.row()));
}

void MainWindow::removeFromPlaylist()
{
	const QModelIndexList items = playlistTableView->selectionModel()->selectedRows();
	QList<qint32> toBeRemoved;

	if(items.isEmpty())
		return;

	for(int i = 0; i < items.size(); i++)
		toBeRemoved.append(playlistModel.getIdByRow(items.at(i).row()));

	mpd.removeSongs(toBeRemoved);
}

void MainWindow::addSelectionToPlaylist()
{
	QStringList files;
	MusicLibraryItem *item;
	MusicLibraryItemSong *songItem;

	// Get selected view indexes
	const QModelIndexList selected = libraryTreeView->selectionModel()->selectedIndexes();
	int selectionSize = selected.size();

	if(selectionSize == 0) {
		QMessageBox::warning(
			this,
			tr("QtMPC: Warning"),
			tr("Couldn't add anything to the playlist because nothing is selected."
			   " Please select something first.")
		);
		return;
	}

	/*
	 * Loop over the selection. Only add files.
	 */
	for(int selectionPos = 0; selectionPos < selectionSize; selectionPos++) {
		const QModelIndex current = selected.at(selectionPos);
		item = static_cast<MusicLibraryItem *>(libraryProxyModel.mapToSource(current).internalPointer());

		switch(item->type()) {
			case MusicLibraryItem::Type_Artist: {
				for(quint32 i = 0; ; i++) {
					const QModelIndex album = current.child(i ,0);
					if (!album.isValid())
						break;

					for(quint32 j = 0; ; j++) {
						const QModelIndex track = album.child(j, 0);
						if (!track.isValid())
							break;
						const QModelIndex mappedSongIndex = libraryProxyModel.mapToSource(track);
						songItem = static_cast<MusicLibraryItemSong *>(mappedSongIndex.internalPointer());
						const QString fileName = songItem->file();
						if(!fileName.isEmpty() && !files.contains(fileName))
							files.append(fileName);
					}
				}
				break;
			}
			case MusicLibraryItem::Type_Album: {
				for(quint32 i = 0; ; i++) {
					QModelIndex track = current.child(i, 0);
					if (!track.isValid())
						break;
					const QModelIndex mappedSongIndex = libraryProxyModel.mapToSource(track);
					songItem = static_cast<MusicLibraryItemSong *>(mappedSongIndex.internalPointer());
					const QString fileName = songItem->file();
					if(!fileName.isEmpty() && !files.contains(fileName))
						files.append(fileName);
				}
				break;
			}
			case MusicLibraryItem::Type_Song: {
				const QString fileName = static_cast<MusicLibraryItemSong *>(item)->file();
				if(!fileName.isEmpty() && !files.contains(fileName))
					files.append(fileName);
				break;
			}
			default:
				break;
		}
	}

	if(!files.isEmpty()) {
		mpd.add(files);
		if(MPDStatus::getInstance()->state() != MPDStatus::State_Playing)
			mpd.startPlayingSong();

		libraryTreeView->selectionModel()->clearSelection();
	}
}

QStringList MainWindow::walkDirView(QModelIndex rootItem)
{
	QStringList files;

	DirViewItem *item = static_cast<DirViewItem *>(dirProxyModel.mapToSource(rootItem).internalPointer());

	if (item->type() == DirViewItem::Type_File) {
		return QStringList(item->fileName());
	}

	for(int i = 0; ; i++) {
		QModelIndex current = rootItem.child(i, 0);
		if (!current.isValid())
			return files;

		QStringList tmp = walkDirView(current);
		for (int j = 0; j < tmp.size(); j++) {
			if (!files.contains(tmp.at(j)))
				files << tmp.at(j);
		}
	}
	return files;
}

void MainWindow::addDirViewSelectionToPlaylist()
{
	QModelIndex current;
	QStringList files;
	DirViewItem *item;

	// Get selected view indexes
	const QModelIndexList selected = dirTreeView->selectionModel()->selectedIndexes();
	int selectionSize = selected.size();

	if(selectionSize == 0) {
		QMessageBox::warning(
			this,
			tr("QtMPC: Warning"),
			tr("Couldn't add anything to the playlist because nothing is selected."
			   " Please select something first.")
		);
		return;
	}

	for(int selectionPos = 0; selectionPos < selectionSize; selectionPos++) {
		current = selected.at(selectionPos);
		item = static_cast<DirViewItem *>(dirProxyModel.mapToSource(current).internalPointer());
		QStringList tmp;

		switch(item->type()) {
			case DirViewItem::Type_Dir:
				tmp = walkDirView(current);
				for (int i = 0; i < tmp.size(); i++) {
					if (!files.contains(tmp.at(i)))
						files << tmp.at(i);
				}
				break;
			case DirViewItem::Type_File:
				if (!files.contains(item->fileName()))
					files << item->fileName();
				break;
			default:
				break;
		}
	}

	if(!files.isEmpty()) {
		mpd.add(files);
		if(MPDStatus::getInstance()->state() != MPDStatus::State_Playing)
			mpd.startPlayingSong();

		dirTreeView->selectionModel()->clearSelection();
	}
}

void MainWindow::libraryItemActivated(const QModelIndex & /*index*/)
{
	addSelectionToPlaylist();
}

void MainWindow::addToPlaylistButtonActivated()
{
	addSelectionToPlaylist();
}

void MainWindow::dirViewAddToPlaylistPushButtonActivated()
{
	addDirViewSelectionToPlaylist();
}

void MainWindow::crossfadingChanged(int seconds)
{
	mpd.setCrossfade(seconds);
}

void MainWindow::updateIntervalChanged(const int mseconds)
{
	statusTimer.setInterval(mseconds);
}

void MainWindow::saveSplitterState(int, int)
{
	QSettings settings;
	settings.setValue("splitterSizes", splitter->saveState());
}

/**
 * Playlist stats are updated.
 * Display it on the bottom of the playlist
 *
 * @param status QString with status
 */
void MainWindow::updatePlayListStatus()
{
	MPDStats * const stats = MPDStats::getInstance();
	QString status = "";

	if (stats->playlistSongs() == 0) {
		playListStatsLabel->setText(status);
		return;
	}

	status = QString::number(stats->playlistArtists());
	if (stats->playlistArtists() == 1) {
		status += " Artist, ";
	} else {
		status += " Artists, ";
	}

	status += QString::number(stats->playlistAlbums());
	if (stats->playlistAlbums() == 1) {
		status += " Album, ";
	} else {
		status += " Albums, ";
	}

	status += QString::number(stats->playlistSongs());
	if (stats->playlistSongs() == 1) {
		status += " Song, ";
	} else {
		status += " Songs, ";
	}

	status += "(";
	status += MPDParseUtils::seconds2formattedString(stats->playlistTime());
	status += ")";

	playListStatsLabel->setText(status);
}

void MainWindow::movePlaylistItems(const QList<quint32> items, const quint32 diff, const int max)
{
	mpd.move(items, diff, max);
}

void MainWindow::addFilenamesToPlaylist(const QStringList filenames, const int row, const int size)
{
	mpd.addid(filenames, row, size);
}
