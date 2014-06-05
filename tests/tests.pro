QT += widgets testlib

HEADERS += src/TestPlane.hpp src/TestGeometry.hpp
SOURCES += src/TestPlane.cpp src/TestGeometry.cpp src/main.cpp

INCLUDEPATH += ../src/
include(../overmix.pri)
