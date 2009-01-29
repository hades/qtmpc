cmake_minimum_required( VERSION 2.6 )

PROJECT( QtMPC )

SET( CMAKE_BUILD_TYPE Debug )

SET( QTMPC_SRCS
	main.cpp
	gui/main_window.cpp
	gui/main_window_playlist.cpp
	gui/main_window_trayicon.cpp
	gui/musiclibraryitem.cpp
	gui/musiclibraryitemroot.cpp
	gui/musiclibraryitemartist.cpp
	gui/musiclibraryitemalbum.cpp
	gui/musiclibraryitemsong.cpp
	gui/musiclibrarymodel.cpp
	gui/musiclibrarysortfiltermodel.cpp
	gui/playlisttablemodel.cpp
	gui/preferences_dialog.cpp
	gui/statistics_dialog.cpp
	gui/dirviewmodel.cpp
	gui/dirviewproxymodel.cpp
	gui/dirviewitem.cpp
	gui/dirviewitemfile.cpp
	gui/dirviewitemdir.cpp
	gui/dirviewitemroot.cpp
	lib/lastfm_metadata_fetcher.cpp
	lib/lastfm_scrobbling.cpp
	lib/mpdconnection.cpp
	lib/mpddatabaseconnection.cpp
	lib/mpdparseutils.cpp
	lib/mpdstats.cpp
	lib/mpdstatus.cpp
	lib/song.cpp
	external/synchttp/synchttp.cpp
)

SET( QTMPC_MOC_HDRS
	gui/main_window.h
	gui/musiclibrarymodel.h
	gui/musiclibrarysortfiltermodel.h
	gui/playlisttablemodel.h
	gui/about_dialog.h
	gui/preferences_dialog.h
	gui/statistics_dialog.h
	gui/dirviewmodel.h
	gui/dirviewproxymodel.h
	lib/lastfm_metadata_fetcher.h
	lib/lastfm_scrobbling.h
	lib/mpdconnection.h
	lib/mpddatabaseconnection.h
	lib/song.h
	external/synchttp/synchttp.h
)

SET( QTMPC_UIS
	gui/about_dialog.ui
)

include_directories( ${CMAKE_SOURCE_DIR}/external/libmaia )
include_directories( ${CMAKE_SOURCE_DIR}/gui )
include_directories( ${CMAKE_SOURCE_DIR}/lib )
include_directories( ${CMAKE_SOURCE_DIR} )

SET( QT_USE_QTNETWORK TRUE )
SET( QT_USE_QTXML TRUE )

# this command finds Qt4 libraries and sets all required variables
FIND_PACKAGE( Qt4 REQUIRED )
FIND_PACKAGE( KDE4 )

IF( NOT DEFINED WANT_KDE_SUPPORT )
	SET( WANT_KDE_SUPPORT ON )
ENDIF( NOT DEFINED WANT_KDE_SUPPORT )

IF( KDE4_FOUND )
	IF ( KDE4_KDECORE_LIBRARY AND KDE4_KDEUI_LIBRARY )
		IF( WANT_KDE_SUPPORT )
			MESSAGE("KDE 4 has been found, will build extra features...")
			ADD_DEFINITIONS( -DENABLE_KDE_SUPPORT )
			SET( ENABLE_KDE_SUPPORT TRUE )
		ELSE( WANT_KDE_SUPPORT )
			MESSAGE("KDE 4 has been found, but won't be used by user request")
		ENDIF( WANT_KDE_SUPPORT )
	ENDIF ( KDE4_KDECORE_LIBRARY AND KDE4_KDEUI_LIBRARY )
ENDIF( KDE4_FOUND )

IF( ENABLE_KDE_SUPPORT )
	include_directories( ${KDE4_INCLUDES} )
	add_definitions( ${KDE4_DEFINITIONS} )

	SET( QTMPC_RCS QtMPC_kde.qrc )

	SET( QTMPC_UIS
		${QTMPC_UIS}
		gui/main_window_kde.ui
		gui/preferences_dialog_kde.ui
		gui/statistics_dialog_kde.ui
	)
ELSE( ENABLE_KDE_SUPPORT )
	SET( QTMPC_SRCS
		${QTMPC_SRCS}
		gui/about_dialog.cpp
	)

	SET( QTMPC_RCS QtMPC.qrc )

	SET( QTMPC_UIS
		${QTMPC_UIS}
		gui/main_window.ui
		gui/preferences_dialog.ui
		gui/statistics_dialog.ui
	)
ENDIF( ENABLE_KDE_SUPPORT )

# enable warnings
ADD_DEFINITIONS( -DQT_NO_DEBUG_OUTPUT )

IF( NOT ENABLE_KDE_SUPPORT )
	ADD_DEFINITIONS( -DQT_NO_STL -DQT_NO_CAST_TO_ASCII -Wall -Wextra )
ENDIF( NOT ENABLE_KDE_SUPPORT )

# add some useful macros and variables
# (QT_USE_FILE is a variable defined by FIND_PACKAGE( Qt4 ) that contains a path to CMake script)
IF( NOT ENABLE_KDE_SUPPORT )
	INCLUDE( ${QT_USE_FILE} )
ENDIF( NOT ENABLE_KDE_SUPPORT )

ADD_SUBDIRECTORY( external/libmaia )

# Compile the resource files
QT4_ADD_RESOURCES( QTMPC_RC_SRCS ${QTMPC_RCS} )

# this will run uic on .ui files:
IF( ENABLE_KDE_SUPPORT )
	KDE4_ADD_UI_FILES( QTMPC_UI_HDRS ${QTMPC_UIS} )
ELSE( ENABLE_KDE_SUPPORT )
	QT4_WRAP_UI( QTMPC_UI_HDRS ${QTMPC_UIS} )
ENDIF( ENABLE_KDE_SUPPORT )

# and finally this will run moc:
IF( NOT ENABLE_KDE_SUPPORT )
	QT4_WRAP_CPP( QTMPC_MOC_SRCS ${QTMPC_MOC_HDRS} )
ENDIF( NOT ENABLE_KDE_SUPPORT )

# we need this to be able to include headers produced by uic in our code
INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR} )

# here we instruct CMake to build "sample" executable from all of the source files
IF( ENABLE_KDE_SUPPORT )
	KDE4_ADD_EXECUTABLE( QtMPC ${QTMPC_SRCS} ${QTMPC_MOC_SRCS} ${QTMPC_RC_SRCS} ${QTMPC_UI_HDRS} )
ELSE( ENABLE_KDE_SUPPORT )
	ADD_EXECUTABLE( QtMPC ${QTMPC_SRCS} ${QTMPC_MOC_SRCS} ${QTMPC_RC_SRCS} ${QTMPC_UI_HDRS} )
ENDIF( ENABLE_KDE_SUPPORT )

# Last but not least, link everything. Based on whether we are using KDE or not.
IF( ENABLE_KDE_SUPPORT )
	TARGET_LINK_LIBRARIES( QtMPC ${QT_LIBRARIES} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} Maia )
ELSE( ENABLE_KDE_SUPPORT )
	TARGET_LINK_LIBRARIES( QtMPC ${QT_LIBRARIES} Maia )
ENDIF( ENABLE_KDE_SUPPORT )

IF( ENABLE_KDE_SUPPORT )
	install( TARGETS QtMPC ${INSTALL_TARGETS_DEFAULT_ARGS} )
ELSE( ENABLE_KDE_SUPPORT )
	install( TARGETS QtMPC
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib )
ENDIF( ENABLE_KDE_SUPPORT )

IF( ENABLE_KDE_SUPPORT )
	install( FILES QtMPCui.rc DESTINATION ${DATA_INSTALL_DIR}/QtMPC )
	install( FILES QtMPC.notifyrc DESTINATION ${DATA_INSTALL_DIR}/QtMPC )
	KDE4_INSTALL_ICONS( ${ICON_INSTALL_DIR} )
ENDIF( ENABLE_KDE_SUPPORT )
