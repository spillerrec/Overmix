cmake_minimum_required(VERSION 3.6)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14") # Enable C++14 features
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall") # Turn on warnings

# Qt compilers setup
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCE_GUI_VIEWER
	colorManager.cpp
	imageCache.cpp
	imageViewer.cpp
	qrect_extras.cpp
	ZoomBox.cpp
	)

# Make sure Windows is set to use the unicode API's
if( WIN32 )
	add_definitions(-DUNICODE -D_UNICODE)
endif( WIN32 )

# Set-up libraries
add_library(qtimgviewer ${SOURCE_GUI_VIEWER} )
target_include_directories(qtimgviewer PRIVATE ${X11_INCLUDE_DIR})
target_link_libraries(qtimgviewer Qt5::Gui Qt5::Widgets Qt5::Concurrent PkgConfig::LCMS2 ${PLATFORM_LIBRARIES})
