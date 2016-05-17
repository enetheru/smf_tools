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

OPTION( DEBUG_IMG "Output debug images, warning there are lots of them :)" OFF )

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
    add_subdirectory(ext/squish)
    set(LIBSQUISH_LIBRARIES "squish")
    include_directories(ext/squish)
endif()
#FIXME look externally for include files if needed
set( LIBS ${LIBS} ${LIBSQUISH_LIBRARIES} )

# we dont want to use external googletest
add_definitions( -DGTEST_USE_OWN_TR1_TUPLE=0 )
add_subdirectory( ext/googletest )
set( GTEST_LIBS "gtest" "gtest_main" )

# header onlylibs
include_directories( ext/elog )
include_directories( ext/optionsparser )

# sub directories
# ===============
add_subdirectory( src )

# Testing
# =======
enable_testing()
add_subdirectory( ctest )
add_subdirectory( gtest )

# CPack
# =====
include( doc/docstrings.cmake )
set( CPACK_STRIP_FILES true )
set( CPACK_GENERATOR "DEB" )
set( CPACK_DEBIAN_PACKAGE_VERSION ${DOC_VERSION} )
set( CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libopenimageio1.3" )
set( CPACK_DEBIAN_PACKAGE_MAINTAINER ${DOC_AUTHORS} )
string( REPLACE "\n" "\n " CPACK_DEBIAN_PACKAGE_DESCRIPTION ${DOC_DESCRIPTION_SHORT} "\n" ${DOC_DESCRIPTION} )
set( CPACK_DEBIAN_PACKAGE_SECTION "utils" )
include( CPack )
