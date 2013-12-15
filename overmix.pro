TEMPLATE = app
TARGET = Overmix

# Libraries used
QT += widgets concurrent
LIBS += -lpng -lz

# C++11 support
QMAKE_CXXFLAGS += -std=c++11

# Use link-time optimization in release builds
Release:QMAKE_CXXFLAGS += -flto
Release:QMAKE_LFLAGS += -flto -O3 #-mtune=native -ftree-vectorize -ftree-slp-vectorize #-ftree-vectorizer-verbose=3 

# Interface
FORMS += src/mainwindow.ui
HEADERS += src/mainwindow.hpp src/imageViewer.hpp
SOURCES += src/mainwindow.cpp src/imageViewer.cpp src/main.cpp
# Core logic
HEADERS += src/MultiPlaneIterator.hpp src/color.hpp src/Plane.hpp src/ImageEx.hpp src/Deteleciner.hpp
SOURCES += src/MultiPlaneIterator.cpp src/color.cpp src/Plane.cpp src/ImageEx.cpp src/Deteleciner.cpp
SOURCES += src/Plane-scaling.cpp src/Plane-edgedetection.cpp src/Plane-blurring.cpp src/Plane-diff.cpp src/Plane-pixel.cpp src/Plane-binarize.cpp
# Aligners
HEADERS += src/AImageAligner.hpp src/AverageAligner.hpp src/AnimatedAligner.hpp src/ImageAligner.hpp src/LayeredAligner.hpp
SOURCES += src/AImageAligner.cpp src/AverageAligner.cpp src/AnimatedAligner.cpp src/ImageAligner.cpp src/LayeredAligner.cpp
# Renders
HEADERS += src/SimpleRender.hpp src/FloatRender.hpp src/DiffRender.hpp src/ARender.hpp
SOURCES += src/SimpleRender.cpp src/FloatRender.cpp src/DiffRender.cpp

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