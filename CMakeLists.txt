cmake_minimum_required( VERSION 2.6 )
project(mapconv)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(OpenImageIO REQUIRED)
find_package(NVTT REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

include_directories(${CMAKE_SOURCE_DIR}/include/tclap-1.2.1/include)
include_directories(${OpenImageIO_INCLUDE_DIRS})
include_directories(${NVTT_INCLUDE_DIRS})

set( LIBS ${LIBS}
	${NVTT_LIBRARY}
	${OPENIMAGEIO_LIBRARY}
	${Boost_LIBRARIES} )

add_subdirectory( src )
