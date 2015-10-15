TEMPLATE = app
TARGET = OvermixCli
CONFIG += console
INCLUDEPATH += ../src

# Libraries used
# QT += widgets KPlotting

include(../overmix.pri)
include(../overmix-cli.pri)

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_DEBUG += -O3

# Code
SOURCES += main.cpp

# Generate both debug and release on Linux
CONFIG += debug_and_release

# Position of binaries and build files
Release:DESTDIR = release
Release:UI_DIR = release/.ui
Release:OBJECTS_DIR = release/.obj
Release:MOC_DIR = release/.moc
Release:RCC_DIR = release/.qrc

Debug:DESTDIR = debug
Debug:UI_DIR = debug/.ui
Debug:OBJECTS_DIR = debug/.obj
Debug:MOC_DIR = debug/.moc
Debug:RCC_DIR = debug/.qrc