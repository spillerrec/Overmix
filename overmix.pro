TEMPLATE = app
TARGET = Overmix

# Libraries used
QT += widgets

include(overmix.pri)

# Interface
FORMS += src/mainwindow.ui
HEADERS += src/mainwindow.hpp src/imageViewer.hpp
SOURCES += src/mainwindow.cpp src/imageViewer.cpp src/main.cpp

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