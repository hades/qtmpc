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

#include "statistics_dialog.h"
#include "mpdstats.h"

#include <QString>
#include <QTime>

#include "mpdparseutils.h"

#ifdef ENABLE_KDE_SUPPORT
StatisticsDialog::StatisticsDialog(QWidget *parent, Qt::WindowFlags f)
		: KDialog(parent, f)
#else
StatisticsDialog::StatisticsDialog(QWidget *parent, Qt::WindowFlags f)
		: QDialog(parent, f)
#endif
{
#ifdef ENABLE_KDE_SUPPORT
	QWidget *widget = new QWidget(this);
	setupUi(widget);

	setMainWidget(widget);

	setButtons(KDialog::Close);
	setCaption("Statistics");
#else
	setupUi(this);
#endif

	MPDStats * const stats = MPDStats::getInstance();

	Artists->setText(QString::number(stats->artists()));
	Albums->setText(QString::number(stats->albums()));
	Songs->setText(QString::number(stats->songs()));

	Uptime->setText(MPDParseUtils::seconds2formattedString(stats->uptime()));
	Playtime->setText(MPDParseUtils::seconds2formattedString(stats->playtime()));
	db_playtime->setText(MPDParseUtils::seconds2formattedString(stats->dbPlaytime()));
	db_update->setText(stats->dbUpdate().toString("d MMMM yyyy hh:mm"));

	PlaylistArtists->setText(QString::number(stats->playlistArtists()));
	PlaylistAlbums->setText(QString::number(stats->playlistAlbums()));
	PlaylistSongs->setText(QString::number(stats->playlistSongs()));
	PlaylistTime->setText(MPDParseUtils::seconds2formattedString(stats->playlistTime()));
}
