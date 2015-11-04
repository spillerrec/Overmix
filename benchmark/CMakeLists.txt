cmake_minimum_required(VERSION 3.0)
project(Overmix)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14") # Enable C++14 features
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall") # Turn on warnings

set(SOURCE_BENCHMARK
	main.cpp
	)

add_executable(OvermixBenchmark ${SOURCE_BENCHMARK})

# Set-up libraries
find_package(Qt5Concurrent REQUIRED)

include_directories ("${PROJECT_SOURCE_DIR}/../src")

if(WIN32)
	set(PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} Shlwapi)
endif()
target_link_libraries(OvermixBenchmark OvermixCore -lbenchmark ${PLATFORM_LIBRARIES})