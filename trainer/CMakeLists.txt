cmake_minimum_required(VERSION 3.0)
project(OvermixTrainer)

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
include_directories ("${PROJECT_SOURCE_DIR}/../src")
include_directories ("${PROJECT_SOURCE_DIR}/../interface/gui/viewer")
target_link_libraries(OvermixTrainer OvermixCore qtimgviewer Qt5::Widgets pugixml::pugixml PkgConfig::LCMS2 ${PLATFORM_LIBRARIES})

# C++14
set_property(TARGET OvermixTrainer PROPERTY CXX_STANDARD 14)
set_property(TARGET OvermixTrainer PROPERTY CXX_STANDARD_REQUIRED ON)
