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
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fopenmp")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	message("gcc")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fopenmp")
else()
	message("Couldn't detect compiler")
endif()

find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
if( UNIX )
	include(FindX11)
	find_package(Qt5X11Extras REQUIRED)
	set(PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} Qt5::X11Extras X11::xcb)
endif( UNIX )
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(JPEG REQUIRED)
find_package(pugixml REQUIRED)
find_package(OpenMP REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBRAW REQUIRED IMPORTED_TARGET libraw)
pkg_check_modules(FFTW3 REQUIRED IMPORTED_TARGET fftw3)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libavutil
)
pkg_check_modules(LCMS2 REQUIRED IMPORTED_TARGET lcms2)

find_library(MATH_LIBRARY m)

find_library(W2XC NAMES w2xc libw2xc HINTS x86_64-w64-mingw32/lib)

if(W2XC)
	message( "Compiling with Waifu2x support" )
	set(WAIFU_LIB ${W2XC})
	set(WAIFU_SOURCES planes/manipulators/Waifu.cpp)
	
	if(EXISTS /usr/share/waifu2x-converter-cpp/)
		add_compile_definitions(WAIFU_MODEL_DIR="/usr/share/waifu2x-converter-cpp/")
	else()
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
