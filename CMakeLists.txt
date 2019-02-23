cmake_minimum_required(VERSION 3.0)
project(Overmix)
add_definitions( "-DOVERMIX_VERSION_STRING=\"Overmix 4.0 dev\"" )

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


# Turn on warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wnon-virtual-dtor -Wcast-align -Wunused") #-Wpedantic


#Fails on Arch for some reason
#find_package(OpenMP REQUIRED)
#set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
#set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	message("clang")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fopenmp")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	message("gcc")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fopenmp")
else()
	message("Couldn't detect compiler")
endif()


find_library(W2XC NAMES w2xc libw2xc HINTS x86_64-w64-mingw32/lib)

if(W2XC)
	message( "Compiling with Waifu2x support" )
	set(WAIFU_LIB ${W2XC})
	set(WAIFU_SOURCES planes/manipulators/Waifu.cpp)
	
	if(EXISTS /usr/share/waifu2x-converter-cpp/)
		add_compile_definitions(WAIFU_MODEL_DIR="/usr/share/waifu2x-converter-cpp/")
	elseif()
		message( "Waifu model dir not found!" )
		add_compile_definitions(WAIFU_MODEL_DIR="models_rgb/")
	endif()
	add_compile_definitions(WAIFU)
else()
	message( "Waifu not found" )
endif()

add_subdirectory(src)
add_subdirectory(interface/gui/viewer)
add_subdirectory(interface)
#add_subdirectory(benchmark)
add_subdirectory(trainer)
