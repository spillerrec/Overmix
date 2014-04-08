# Libraries used
QT += concurrent
LIBS += -lpng -lz

# C++11 support
QMAKE_CXXFLAGS += -std=c++11

# Core logic
HEADERS += $$PWD/src/MultiPlaneIterator.hpp $$PWD/src/color.hpp $$PWD/src/Plane.hpp $$PWD/src/ImageEx.hpp $$PWD/src/Deteleciner.hpp
SOURCES += $$PWD/src/MultiPlaneIterator.cpp $$PWD/src/color.cpp $$PWD/src/Plane.cpp $$PWD/src/ImageEx.cpp $$PWD/src/Deteleciner.cpp
SOURCES += $$PWD/src/Plane-scaling.cpp $$PWD/src/Plane-edgedetection.cpp $$PWD/src/Plane-blurring.cpp $$PWD/src/Plane-diff.cpp $$PWD/src/Plane-pixel.cpp $$PWD/src/Plane-binarize.cpp

# Aligners
HEADERS += $$PWD/src/AImageAligner.hpp $$PWD/src/AverageAligner.hpp $$PWD/src/AnimatedAligner.hpp $$PWD/src/ImageAligner.hpp $$PWD/src/LayeredAligner.hpp
SOURCES += $$PWD/src/AImageAligner.cpp $$PWD/src/AverageAligner.cpp $$PWD/src/AnimatedAligner.cpp $$PWD/src/ImageAligner.cpp $$PWD/src/LayeredAligner.cpp

# Renders
HEADERS += $$PWD/src/SimpleRender.hpp $$PWD/src/FloatRender.hpp $$PWD/src/DiffRender.hpp $$PWD/src/PlaneRender.hpp $$PWD/src/DifferenceRender.hpp $$PWD/src/ARender.hpp
SOURCES += $$PWD/src/SimpleRender.cpp $$PWD/src/FloatRender.cpp $$PWD/src/DiffRender.cpp $$PWD/src/PlaneRender.cpp $$PWD/src/DifferenceRender.cpp

# mics
HEADERS += $$PWD/src/debug.hpp $$PWD/src/AnimationSaver.hpp 
SOURCES += $$PWD/src/debug.cpp $$PWD/src/AnimationSaver.cpp 
