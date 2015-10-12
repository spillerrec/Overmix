TEMPLATE = app
TARGET = Overmix
target.path = /usr/local/bin
INSTALLS += target

# Libraries used
QT += widgets printsupport
LIBS += -llcms2
unix{
	QT += x11extras
	LIBS += -lxcb
}

include(overmix.pri)

# GUI interface
FORMS   += src/gui/mainwindow.ui
HEADERS += src/gui/mainwindow.hpp src/gui/ImagesModel.hpp src/gui/FullscreenViewer.hpp
SOURCES += src/gui/mainwindow.cpp src/gui/ImagesModel.cpp src/gui/FullscreenViewer.cpp src/main.cpp

# CLI interface
HEADERS += src/cli/CommandParser.hpp src/cli/Processor.hpp src/cli/Parsing.hpp
SOURCES += src/cli/CommandParser.cpp src/cli/Processor.cpp

# Configurators
FORMS   += src/gui/configs/ImageAligner.ui
HEADERS += src/gui/configs/AlignerConfigs.hpp src/gui/configs/RenderConfigs.hpp src/gui/configs/AConfig.hpp src/gui/configs/ConfigChooser.hpp
SOURCES += src/gui/configs/AlignerConfigs.cpp src/gui/configs/RenderConfigs.cpp

# File savers
FORMS   += src/gui/savers/DumpSaver.ui
HEADERS += src/gui/savers/DumpSaver.hpp src/gui/savers/ASaver.hpp
SOURCES += src/gui/savers/DumpSaver.cpp

# Visualisation
HEADERS += src/gui/visualisations/QCustomPlot/qcustomplot.h
SOURCES += src/gui/visualisations/QCustomPlot/qcustomplot.cpp
HEADERS += src/gui/visualisations/MovementGraph.hpp
SOURCES += src/gui/visualisations/MovementGraph.cpp


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

# Increase optimization level for release builds
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3

unix{
	# Enable sanitizer in debug mode
	QMAKE_CXXFLAGS_DEBUG += -fsanitize=address -fno-omit-frame-pointer
	QMAKE_LFLAGS_DEBUG   += -fsanitize=address
}

# Use link-time optimization in release builds
# Release:QMAKE_CXXFLAGS += -flto
# Release:QMAKE_LFLAGS += -flto -O3 #-mtune=native -ftree-vectorize -ftree-slp-vectorize #-ftree-vectorizer-verbose=3 

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