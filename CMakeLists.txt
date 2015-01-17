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

set( LIBS ${LIBS}
    ${LIBSQUISH_LIBRARIES}
	${OPENIMAGEIO_LIBRARY}
	${Boost_LIBRARIES} )

if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Debug CACHE STRING
        "Choose the type of build, options are None, Debug, Release, RelWithDebInfo, MinSizeRel" FORCE )
endif()

set( CMAKE_CXX_FLAGS "-Wall -std=c++11" )
set( DEBUG_IMG OFF CACHE BOOL "Output debug images, warning there are lots of them :)" )

add_subdirectory( src )
