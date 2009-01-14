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

#ifndef STATISTICS_DIALOG_H
#define STATISTICS_DIALOG_H

#ifdef ENABLE_KDE_SUPPORT
#include <KDialog>
#include "ui_statistics_dialog_kde.h"
#else
#include <QDialog>
#include "ui_statistics_dialog.h"
#endif

#include "mpdstats.h"

#ifdef ENABLE_KDE_SUPPORT
class StatisticsDialog : public KDialog, private Ui::StatisticsDialog
#else
class StatisticsDialog : public QDialog, private Ui::StatisticsDialog
#endif
{
	Q_OBJECT

	public:
		StatisticsDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);

	private:
		QString seconds2formattedString(quint32 seconds);
};

#endif
