cmake_minimum_required(VERSION 3.0)
project(Overmix)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14") # Enable C++14 features
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall") # Turn on warnings

# Qt compilers setup
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCE_CLI
	cli/CommandParser.cpp
	cli/Processor.cpp
	)

set(SOURCE_GUI
	gui/mainwindow.cpp
	gui/ImagesModel.cpp
	gui/FullscreenViewer.cpp
	)

set(SOURCE_GUI_CONFIGS
	gui/configs/AlignerConfigs.cpp
	gui/configs/RenderConfigs.cpp
	)

set(SOURCE_GUI_SAVERS
	gui/savers/DumpSaver.cpp
	)

set(SOURCE_GUI_VISUALISATION
	gui/visualisations/QCustomPlot/qcustomplot.cpp
	gui/visualisations/MovementGraph.cpp
	)

# Viewer module #TODO: move to its own sub-folder?
if( WIN32 ) # Don't use registry on windows for saving options
	add_definitions( -DPORTABLE )
endif( WIN32 )
set(SOURCE_GUI_VIEWER
	gui/viewer/colorManager.cpp
	gui/viewer/imageCache.cpp
	gui/viewer/imageViewer.cpp
	gui/viewer/qrect_extras.cpp
	)

add_executable(OvermixCli ${SOURCE_CLI} main-cli.cpp)
add_executable(Overmix WIN32 ${SOURCE_CLI} ${SOURCE_GUI} ${SOURCE_GUI_CONFIGS} ${SOURCE_GUI_SAVERS} ${SOURCE_GUI_VISUALISATION} ${SOURCE_GUI_VIEWER} ${extra_moc} main.cpp)

# Set-up libraries
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
if( UNIX )
	find_package(Qt5X11Extras REQUIRED)
	set(PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} Qt5::X11Extras -lxcb)
endif( UNIX )

include_directories ("${PROJECT_SOURCE_DIR}/../src")
target_link_libraries(OvermixCli OvermixCore Qt5::Widgets)
target_link_libraries(Overmix OvermixCore Qt5::Widgets Qt5::PrintSupport -llcms2 ${PLATFORM_LIBRARIES})