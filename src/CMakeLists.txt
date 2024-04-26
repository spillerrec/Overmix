cmake_minimum_required(VERSION 3.6)

find_path(PNGPP_INCLUDE_DIRS "png++/color.hpp")

set(SOURCES_CONTAINERS
	containers/AContainer.cpp
	containers/ImageContainer.cpp
	containers/ImageContainerSaver.cpp
	)

set(SOURCES_PLANES
	planes/basic/difference.cpp
	planes/basic/rotation.cpp
	planes/manipulators/Inpaint.cpp
	planes/manipulators/Mosaic.cpp
	planes/manipulators/Steam.cpp
	planes/ColorSpace.cpp
	planes/DistanceMatrix.cpp
	planes/ImageEx.cpp
	planes/ImageEx-dump.cpp
	planes/ImageEx-jpeg.cpp
	planes/ImageEx-libraw.cpp
	planes/ImageEx-png.cpp
	planes/ImageEx-qimage.cpp
	planes/FourierPlane.cpp
	planes/JpegImage.cpp
	planes/Plane.cpp
	planes/Plane-scaling.cpp
	planes/Plane-edgedetection.cpp
	planes/Plane-blurring.cpp
	planes/Plane-pixel.cpp
	planes/Plane-binarize.cpp
	planes/PorterDuff.cpp
	planes/PlaneExcept.cpp
	planes/PatternRemove.cpp
	planes/gwenview/iodevicejpegsourcemanager.cpp
	)
	
set(SOURCES_ALIGNERS
	aligners/AverageAligner.cpp
	aligners/ClusterAligner.cpp
	aligners/RecursiveAligner.cpp
	aligners/AnimationSeparator.cpp
	aligners/FrameAligner.cpp
	aligners/NearestFrameAligner.cpp
	aligners/LinearAligner.cpp
	aligners/SuperResAligner.cpp
	aligners/IndependentAligner.cpp
	)
	
set(SOURCES_COMPARATORS
	comparators/AComparator.cpp
	comparators/BruteForceComparator.cpp
	comparators/GradientComparator.cpp
	comparators/GradientPlane.cpp
	comparators/MultiScaleComparator.cpp
	comparators/LogPolarComparator.cpp
	)
	
set(SOURCES_RENDERS
	renders/AnimRender.cpp
	renders/AverageRender.cpp
	renders/FastRender.cpp
	renders/FocusStackingRender.cpp
	renders/FloatRender.cpp
	renders/DiffRender.cpp
	renders/DistanceMatrixRender.cpp
	renders/PlaneRender.cpp
	renders/StatisticsRender.cpp
	renders/RobustSrRender.cpp
	renders/EstimatorRender.cpp
	renders/JpegRender.cpp
	renders/JpegConstrainerRender.cpp
	)
	
set(SOURCES_DEGRADERS
	degraders/JpegDegrader.cpp
	)
	
set(SOURCES_VIDEO
	video/Video.cpp
	video/VideoFrame.cpp
	video/VideoStream.cpp
	)
	
set(SOURCES_OTHERS
	aligners/AnimationSaver.cpp
	planes/dump/DumpPlane.cpp
	utils/Animator.cpp
	utils/ImageLoader.cpp
	utils/SRSampleCreator.cpp
	color.cpp
	debug.cpp
	Deteleciner.cpp
	MultiPlaneIterator.cpp
	)

add_library(OvermixCore 
	${SOURCES_CONTAINERS} 
	${SOURCES_PLANES} 
	${SOURCES_ALIGNERS} 
	${SOURCES_COMPARATORS} 
	${SOURCES_RENDERS} 
	${SOURCES_DEGRADERS} 
	${SOURCES_VIDEO} 
	${WAIFU_SOURCES} 
	${SOURCES_OTHERS} 
	)
target_include_directories(OvermixCore PRIVATE ${JPEG_INCLUDE_DIRS} ${PNGPP_INCLUDE_DIRS})
target_link_libraries(OvermixCore Qt5::Gui Qt5::Concurrent PNG::PNG ZLIB::ZLIB LibLZMA::LibLZMA pugixml::pugixml PkgConfig::FFTW3 PkgConfig::LIBRAW PkgConfig::FFMPEG OpenMP::OpenMP_CXX ${JPEG_LIBRARIES} ${WAIFU_LIB})
if(MATH_LIBRARY)
    target_link_libraries(OvermixCore ${MATH_LIBRARY})
endif()

# C++17
set_property(TARGET OvermixCore PROPERTY CXX_STANDARD 17)
set_property(TARGET OvermixCore PROPERTY CXX_STANDARD_REQUIRED ON)
