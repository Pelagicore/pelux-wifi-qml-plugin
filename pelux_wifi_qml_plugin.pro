TARGET = $$qtLibraryTarget(wifi_backend)
TEMPLATE = lib
CONFIG += plugin

QT += core ivicore dbus
#QT_FOR_CONFIG += ivicore
#!qtConfig(ivigenerator): error("No ivigenerator available: Make sure QtIvi is installed and configured correctly")

include($$SOURCE_DIR/config.pri)

LIBS += -L$$LIB_DESTDIR -l$$qtLibraryTarget(Connectivity)
DESTDIR = $$BUILD_DIR/qtivi

QML_IMPORT_PATH = $$OUT_PWD/qml

PLUGIN_TYPE = qtivi

INCLUDEPATH += $$OUT_PWD/../connectivity

SOURCES += wifibackend.cpp \
           connectivityplugin.cpp \
           userinputagent.cpp

HEADERS += wifibackend.h \
           connectivityplugin.h \
           userinputagent.h

QMAKE_RPATHDIR += $$QMAKE_REL_RPATH_BASE/$$relative_path($$INSTALL_PREFIX/neptune3/lib, $$INSTALL_PREFIX/neptune3/qtivi)

target.path = $$INSTALL_PREFIX/neptune3/qtivi
INSTALLS += target
