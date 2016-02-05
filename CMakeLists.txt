cmake_minimum_required( VERSION 2.6 )

project(mapconv)

# Configuration
# =============
if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Debug CACHE STRING
        "The type of build, options are None, Debug, Release, RelWithDebInfo, MinSizeRel" FORCE )
endif()
message( "Build Type: " ${CMAKE_BUILD_TYPE} )

if( NOT DEFINED BUILD_DOCS )
    set( BUILD_DOCS true CACHE BOOL
        "Whether or not to build the documentation"
        FORCE )
endif()
message( "Build Documentation: " ${BUILD_DOCS} )

set( DEBUG_IMG OFF CACHE BOOL "Output debug images, warning there are lots of them :)" )

# Compiler
# --------
set( CMAKE_CXX_FLAGS "-Wall -std=c++11" )

# CMake
# -----
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Dependencies
# ============
find_package(OpenImageIO REQUIRED)
include_directories(${OpenImageIO_INCLUDE_DIRS})
set( LIBS ${LIBS} ${OPENIMAGEIO_LIBRARY} )

find_package(Boost REQUIRED COMPONENTS system)
set( LIBS ${LIBS} ${Boost_LIBRARIES} )

find_package(libSquish)

if (NOT LIBSQUISH_FOUND)
	add_subdirectory(src/squish)
	set(LIBSQUISH_LIBRARIES "squish")
	include_directories(src/squish)
endif()

set( LIBS ${LIBS} ${LIBSQUISH_LIBRARIES} )

# sub directories
# ===============
add_subdirectory( src )

# CPack
# =====
include( doc/docstrings.cmake )
set( CPACK_STRIP_FILES true )
set( CPACK_GENERATOR "DEB" )
set( CPACK_DEBIAN_PACKAGE_VERSION "1.0" )
#set( CPACK_DEBIAN_PACKAGE_DEPENDS <debian package list goes here> )
set( CPACK_DEBIAN_PACKAGE_MAINTAINER $DOC_AUTHORS )
string( REPLACE "\n" "\n " CPACK_DEBIAN_PACKAGE_DESCRIPTION ${DOC_DESCRIPTION_SHORT} "\n" ${DOC_DESCRIPTION} )
set( CPACK_DEBIAN_PACKAGE_SECTION "utils" )
include( CPack )
