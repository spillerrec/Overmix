cmake_minimum_required(VERSION 3.0)
project(OvermixTrainer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14") # Enable C++14 features
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall") # Turn on warnings

# Qt compilers setup
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCE_TRAINER
	MainWindow.cpp
	Slide.cpp
	)


add_executable(OvermixTrainer WIN32 ${SOURCE_TRAINER} main.cpp)

# Set-up libraries
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)
if( UNIX )
	find_package(Qt5X11Extras REQUIRED)
	set(PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} Qt5::X11Extras -lxcb)
endif( UNIX )

include_directories ("${PROJECT_SOURCE_DIR}/../src")
include_directories ("${PROJECT_SOURCE_DIR}/../interface/gui/viewer")
target_link_libraries(OvermixTrainer OvermixCore ImgViewer Qt5::Widgets -lpugixml -llcms2 ${PLATFORM_LIBRARIES})