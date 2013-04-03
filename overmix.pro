TEMPLATE = app
TARGET = Overmix
DEPENDPATH += . src
INCLUDEPATH += .
QT += widgets concurrent
LIBS += -lpng

# Input
HEADERS += src/mainwindow.hpp src/MultiImage.hpp src/imageViewer.hpp src/MultiPlaneIterator.hpp src/color.hpp src/Image.hpp src/Plane.hpp src/ImageEx.hpp
FORMS += src/mainwindow.ui
SOURCES += src/main.cpp src/mainwindow.cpp src/MultiImage.cpp src/imageViewer.cpp src/MultiPlaneIterator.cpp src/image.cpp src/Plane.cpp src/ImageEx.cpp
