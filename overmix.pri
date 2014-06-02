# Libraries used
QT += concurrent
LIBS += -lpng -lz -llzma

# C++11 support
QMAKE_CXXFLAGS += -std=c++11

# Core logic
HEADERS += $$PWD/src/MultiPlaneIterator.hpp $$PWD/src/color.hpp $$PWD/src/ImageEx.hpp $$PWD/src/Deteleciner.hpp
SOURCES += $$PWD/src/MultiPlaneIterator.cpp $$PWD/src/color.cpp $$PWD/src/ImageEx.cpp $$PWD/src/Deteleciner.cpp

# Planes
HEADERS += $$PWD/src/planes/Plane.hpp $$PWD/src/planes/PlaneBase.hpp
SOURCES += $$PWD/src/planes/Plane.cpp
SOURCES += $$PWD/src/planes/Plane-scaling.cpp $$PWD/src/planes/Plane-edgedetection.cpp $$PWD/src/planes/Plane-blurring.cpp $$PWD/src/planes/Plane-diff.cpp $$PWD/src/planes/Plane-pixel.cpp $$PWD/src/planes/Plane-binarize.cpp

# Aligners
HEADERS += $$PWD/src/AImageAligner.hpp $$PWD/src/AverageAligner.hpp $$PWD/src/AnimatedAligner.hpp $$PWD/src/ImageAligner.hpp $$PWD/src/LayeredAligner.hpp $$PWD/src/RecursiveAligner.hpp
SOURCES += $$PWD/src/AImageAligner.cpp $$PWD/src/AverageAligner.cpp $$PWD/src/AnimatedAligner.cpp $$PWD/src/ImageAligner.cpp $$PWD/src/LayeredAligner.cpp $$PWD/src/RecursiveAligner.cpp

# Renders
HEADERS += $$PWD/src/renders/SimpleRender.hpp $$PWD/src/renders/FloatRender.hpp $$PWD/src/renders/DiffRender.hpp $$PWD/src/renders/PlaneRender.hpp $$PWD/src/renders/DifferenceRender.hpp $$PWD/src/renders/ARender.hpp
SOURCES += $$PWD/src/renders/SimpleRender.cpp $$PWD/src/renders/FloatRender.cpp $$PWD/src/renders/DiffRender.cpp $$PWD/src/renders/PlaneRender.cpp $$PWD/src/renders/DifferenceRender.cpp

# mics
HEADERS += $$PWD/src/debug.hpp $$PWD/src/AnimationSaver.hpp $$PWD/src/dump/DumpPlane.hpp $$PWD/src/ARenderPipe.hpp $$PWD/src/RenderOperations.hpp
SOURCES += $$PWD/src/debug.cpp $$PWD/src/AnimationSaver.cpp $$PWD/src/dump/DumpPlane.cpp
