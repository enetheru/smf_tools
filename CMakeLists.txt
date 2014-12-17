cmake_minimum_required( VERSION 2.6 )
project(mapconv)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(OpenImageIO REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(libSquish)

if (NOT LIBSQUISH_FOUND)
	add_subdirectory(src/squish)
	set(LIBSQUISH_LIBRARIES "squish")
	include_directories(src/squish)
endif()

include_directories(${OpenImageIO_INCLUDE_DIRS})
include_directories(${NVTT_INCLUDE_DIRS})

set( LIBS ${LIBS}
	${OPENIMAGEIO_LIBRARY}
	${Boost_LIBRARIES}
    ${LIBSQUISH_LIBRARIES} )

set( CMAKE_CXX_FLAGS "-g -Wall -std=c++11" )
set( DEBUG_IMG OFF CACHE BOOL "Output debug images, warning there are lots of them :)" )

add_subdirectory( src )
