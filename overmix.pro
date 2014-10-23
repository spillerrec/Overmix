TEMPLATE = app
TARGET = Overmix

# Libraries used
QT += widgets
LIBS += -llcms2
unix{
	QT += x11extras
	LIBS += -lxcb
}

include(overmix.pri)

QMAKE_CXXFLAGS_DEBUG += -O2

# Interface
FORMS   += src/gui/mainwindow.ui
HEADERS += src/gui/mainwindow.hpp src/gui/ImagesModel.hpp
SOURCES += src/gui/mainwindow.cpp src/gui/ImagesModel.cpp src/main.cpp

#Viewer
HEADERS += src/gui/viewer/colorManager.h \
           src/gui/viewer/imageCache.h \
           src/gui/viewer/imageViewer.h \
           src/gui/viewer/qrect_extras.h
SOURCES += src/gui/viewer/colorManager.cpp \
           src/gui/viewer/imageCache.cpp \
           src/gui/viewer/imageViewer.cpp \
           src/gui/viewer/qrect_extras.cpp
win32{
	DEFINES += PORTABLE
}

# Use link-time optimization in release builds
Release:QMAKE_CXXFLAGS += -flto
Release:QMAKE_LFLAGS += -flto -O3 #-mtune=native -ftree-vectorize -ftree-slp-vectorize #-ftree-vectorizer-verbose=3 

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