TEMPLATE = app
TARGET = Overmix
DEPENDPATH += . src
INCLUDEPATH += .
QT += widgets concurrent
LIBS += -lpng16

# Input
HEADERS += src/mainwindow.h src/MultiImage.h src/imageViewer.h src/MultiImageIterator.h src/color.h src/image.h src/Plane.hpp src/ImageEx.hpp
FORMS += src/mainwindow.ui
SOURCES += src/main.cpp src/mainwindow.cpp src/MultiImage.cpp src/imageViewer.cpp src/MultiImageIterator.cpp src/image.cpp src/Plane.cpp src/ImageEx.cpp
