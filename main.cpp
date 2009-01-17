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
#include <KApplication>
#include <KAboutData>
#include <KCmdLineArgs>
#else
#include <QApplication>
#endif

#include "QtMPC_config.h"
#include "gui/main_window.h"

int main(int argc, char *argv[])
{
#ifdef ENABLE_KDE_SUPPORT
	KAboutData aboutData( qPrintable(PACKAGE_NAME), 0,
		ki18n(qPrintable(PACKAGE_NAME)), qPrintable(PACKAGE_VERSION),
		ki18n("A Qt interface to MPD"),
		KAboutData::License_GPL_V2,
		ki18n("Copyright (C) 2007-2009  The QtMPC Authors"),
		ki18n(""),
		"http://qtmpc.lowblog.nl/",
		""
	);

	aboutData.addAuthor(ki18n("Sander Knopper"), ki18n(""), "", "http://www.knopper.tk/");
	aboutData.addAuthor(ki18n("Roeland Douma"), ki18n(""), "", "http://rullzer.com/");

	KCmdLineArgs::init( argc, argv, &aboutData );

	KApplication app;
#else
	QApplication app(argc, argv);
	QApplication::setApplicationName(PACKAGE_NAME);
	QApplication::setOrganizationName("lowblogprojects");
#endif

	MainWindow w;

	return app.exec();
}

