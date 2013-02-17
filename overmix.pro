TEMPLATE = app
TARGET = Overmix
DEPENDPATH += . src
INCLUDEPATH += .
QT += widgets

# Input
HEADERS += src/mainwindow.h src/MultiImage.h src/imageViewer.h src/MultiImageIterator.h src/color.h src/image.h
FORMS += src/mainwindow.ui
SOURCES += src/main.cpp src/mainwindow.cpp src/MultiImage.cpp src/imageViewer.cpp src/MultiImageIterator.cpp src/image.cpp
