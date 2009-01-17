TEMPLATE = app
TARGET =
DEPENDPATH += . gui lib
INCLUDEPATH += . lib ./external/libmaia/
CONFIG += qt
QT += network xml
RESOURCES = QtMPC.qrc
#DEFINES += QT_NO_DEBUG_OUTPUT

# Input
HEADERS += external/libmaia/maiaObject.h \
		external/libmaia/maiaFault.h \
		external/libmaia/maiaXmlRpcClient.h \
		gui/main_window.h \
		gui/musiclibraryitem.h \
		gui/musiclibraryitemroot.h \
		gui/musiclibraryitemartist.h \
		gui/musiclibraryitemalbum.h \
		gui/musiclibraryitemsong.h \
		gui/musiclibrarymodel.h \
		gui/musiclibrarysortfiltermodel.h \
		gui/playlisttablemodel.h \
		gui/about_dialog.h \
		gui/preferences_dialog.h \
		gui/statistics_dialog.h \
		gui/dirviewmodel.h \
		gui/dirviewproxymodel.h \
		gui/dirviewitem.h \
		gui/dirviewitemdir.h \
		gui/dirviewitemfile.h \
		gui/dirviewitemroot.h \
		lib/lastfm_metadata_fetcher.h \
		lib/lastfm_scrobbling.h \
		lib/mpdconnection.h \
		lib/mpddatabaseconnection.h \
		lib/mpdparseutils.h \
		lib/mpdstats.h \
		lib/mpdstatus.h \
		lib/song.h \
		external/synchttp/synchttp.h \
		QtMPC_config.h

FORMS += gui/main_window.ui \
		gui/about_dialog.ui \
		gui/preferences_dialog.ui \
		gui/statistics_dialog.ui

SOURCES += main.cpp \
		external/libmaia/maiaObject.cpp \
		external/libmaia/maiaFault.cpp \
		external/libmaia/maiaXmlRpcClient.cpp \
		gui/main_window.cpp \
		gui/main_window_playlist.cpp \
		gui/main_window_trayicon.cpp \
		gui/musiclibraryitem.cpp \
		gui/musiclibraryitemroot.cpp \
		gui/musiclibraryitemartist.cpp \
		gui/musiclibraryitemalbum.cpp \
		gui/musiclibraryitemsong.cpp \
		gui/musiclibrarymodel.cpp \
		gui/musiclibrarysortfiltermodel.cpp \
		gui/playlisttablemodel.cpp \
		gui/about_dialog.cpp \
		gui/preferences_dialog.cpp \
		gui/statistics_dialog.cpp \
		gui/dirviewmodel.cpp \
		gui/dirviewproxymodel.cpp \
		gui/dirviewitem.cpp \
		gui/dirviewitemfile.cpp \
		gui/dirviewitemdir.cpp \
		gui/dirviewitemroot.cpp \
		lib/lastfm_metadata_fetcher.cpp \
		lib/lastfm_scrobbling.cpp \
		lib/mpdconnection.cpp \
		lib/mpddatabaseconnection.cpp \
		lib/mpdparseutils.cpp \
		lib/mpdstats.cpp \
		lib/mpdstatus.cpp \
		lib/song.cpp \
		external/synchttp/synchttp.cpp
