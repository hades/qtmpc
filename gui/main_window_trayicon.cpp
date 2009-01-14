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

#ifdef ENABLE_KDE_SUPPORT
#include <KAction>
#include <KApplication>
#include <KStandardAction>
#endif

#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QKeyEvent>
#include <QHeaderView>

#include "main_window.h"

bool MainWindow::setupTrayIcon()
{
	if (!QSystemTrayIcon::isSystemTrayAvailable()) {
		trayIcon = NULL;
		return false;
	}

	trayIcon = new QSystemTrayIcon(this);
	trayIcon->installEventFilter(volumeSliderEventHandler);
	trayIconMenu = new QMenu(this);

	//Setup Actions
	playPauseAction = new QAction(tr("&Play"), trayIconMenu);
	playPauseAction->setIcon(playbackPlay);
	connect(playPauseAction, SIGNAL(triggered()), this, SLOT(playPauseTrack()));

	stopAction = new QAction(tr("&Stop"), trayIconMenu);
	stopAction->setIcon(playbackStop);
	connect(stopAction, SIGNAL(triggered()), this, SLOT(stopTrack()));

	prevAction = new QAction(tr("P&rev"), trayIconMenu);
	prevAction->setIcon(playbackPrev);
	connect(prevAction, SIGNAL(triggered()), this, SLOT(previousTrack()));

	nextAction = new QAction(tr("&Next"), trayIconMenu);
	nextAction->setIcon(playbackNext);
	connect(nextAction, SIGNAL(triggered()), this, SLOT(nextTrack()));

#ifdef ENABLE_KDE_SUPPORT
	quitAction = KStandardAction::quit(kapp, SLOT(quit()), trayIconMenu);
#else
	quitAction = new QAction(tr("&Quit"), trayIconMenu);
	connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
#endif

	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconClicked(QSystemTrayIcon::ActivationReason)));

	//Setup Menu
	trayIconMenu->addAction(prevAction);
	trayIconMenu->addAction(nextAction);
	trayIconMenu->addAction(stopAction);
	trayIconMenu->addAction(playPauseAction);
	trayIconMenu->addSeparator();
	trayIconMenu->addAction(quitAction);

	trayIcon->setContextMenu(trayIconMenu);
	trayIcon->setIcon(icon);
	trayIcon->setToolTip("QtMPC");

	return true;
}

void MainWindow::trayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason) {
		case QSystemTrayIcon::Unknown:
		case QSystemTrayIcon::Context:
		case QSystemTrayIcon::DoubleClick:
			break;
		case QSystemTrayIcon::Trigger:
			if (isHidden()) {
				showNormal();
			} else {
				hide();
			}
			break;
		case QSystemTrayIcon::MiddleClick:
			break;
	}
}
