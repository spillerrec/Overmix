cmake_minimum_required(VERSION 3.0)

# C++14
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

find_package(Qt5Gui REQUIRED)
find_package(Qt5Concurrent REQUIRED)

# Turn on warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(SOURCES_CONTAINERS
	containers/AContainer.cpp
	containers/ImageContainer.cpp
	)

set(SOURCES_PLANES
	planes/ImageEx.cpp
	planes/Plane.cpp
	planes/FourierPlane.cpp
	planes/Plane-scaling.cpp
	planes/Plane-edgedetection.cpp
	planes/Plane-blurring.cpp
	planes/Plane-diff.cpp
	planes/Plane-pixel.cpp
	planes/Plane-binarize.cpp
	planes/ImageEx-dump.cpp
	planes/ImageEx-jpeg.cpp
	planes/ImageEx-png.cpp
	planes/ImageEx-qimage.cpp
	planes/gwenview/iodevicejpegsourcemanager.cpp
	)
	
set(SOURCES_CONTAINER_SAVER
	containers/ImageContainerSaver.cpp
	containers/pugixml/pugixml.cpp
	)
	
set(SOURCES_ALIGNERS
	aligners/AImageAligner.cpp
	aligners/AverageAligner.cpp
	aligners/ImageAligner.cpp
	aligners/LayeredAligner.cpp
	aligners/RecursiveAligner.cpp
	aligners/AnimationSeparator.cpp
	aligners/FrameAligner.cpp
	aligners/LinearAligner.cpp
	aligners/SuperResAligner.cpp
	)
	
set(SOURCES_RENDERS
	renders/AnimRender.cpp
	renders/AverageRender.cpp
	renders/FloatRender.cpp
	renders/DiffRender.cpp
	renders/PlaneRender.cpp
	renders/StatisticsRender.cpp
	renders/PixelatorRender.cpp
	renders/RobustSrRender.cpp
	renders/EstimatorRender.cpp
	renders/JpegRender.cpp
	)
	
set(SOURCES_DEGRADERS
	degraders/JpegDegrader.cpp
	)
	
set(SOURCES_OTHERS
	aligners/AnimationSaver.cpp
	planes/dump/DumpPlane.cpp
	utils/Animator.cpp
	utils/ImageLoader.cpp
	color.cpp
	debug.cpp
	Deteleciner.cpp
	MultiPlaneIterator.cpp
	)

# mingw uses a different naming for FFTW apparently # TODO: find a better solution
if( WIN32 )
	set(PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} -lfftw3-3)
else( WIN32 )
	set(PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} -lfftw3)
endif( WIN32 )

add_library(OvermixCore ${SOURCES_CONTAINERS} ${SOURCES_PLANES} ${SOURCES_CONTAINER_SAVER} ${SOURCES_ALIGNERS} ${SOURCES_RENDERS} ${SOURCES_DEGRADERS} ${SOURCES_OTHERS} )
target_link_libraries(OvermixCore Qt5::Gui Qt5::Concurrent -lm -lpng -ljpeg -lz -llzma ${PLATFORM_LIBRARIES})
