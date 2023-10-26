cmake_minimum_required( VERSION 3.24.0 )

project(mapconv)

option( CMAKE_VERBOSE_MAKEFILE "Enable Verbose Makefile generation" ON)
option( BUILD_DOCS "Whether or not to build the documentation" OFF )
option( BUILD_TESTS "Test" OFF )
option( BUILD_PACKAGES "Build Packages" OFF )
option( DEBUG_IMG "Output debug images, warning there are lots of them :)" OFF )

# Configuration
# =============
if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Debug CACHE STRING
        "The type of build, options are None, Debug, Release, RelWithDebInfo, MinSizeRel" FORCE )
endif()
message( "Build Type: " ${CMAKE_BUILD_TYPE} )

# Compiler
# --------
set( CMAKE_CXX_FLAGS "-Wall -std=c++20 -fstack-usage" )

# CMake
# -----
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Dependencies
# ============
find_package( CMath REQUIRED )
find_package( OpenImageIO REQUIRED )
find_package( Boost REQUIRED COMPONENTS thread filesystem )
find_package( OpenEXR REQUIRED )
find_package( libjpeg-turbo REQUIRED )
find_package( ZLIB REQUIRED )
find_package( TIFF REQUIRED HINTS C:/src/libTIFF/cmake-build-debug/lib/cmake/tiff/)
find_package( PNG REQUIRED HINTS C:/src/lpng1640/cmake-build-debug/lib/cmake/libpng )
find_package( fmt REQUIRED HINTS C:/src/fmt/cmake-build-debug/lib/cmake/fmt)
find_package( spdlog REQUIRED HINTS C:/src/spdlog/cmake-build-debug/lib/cmake/spdlog )
find_package( libsquish REQUIRED HINTS C:/src/libsquish/cmake-build-debug/lib/cmake/libsquish )

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

# Testing
# =======
if( BUILD_TESTS )
    enable_testing()
    find_package( GTest REQUIRED HINTS C:/src/googletest/cmake-build-debug/lib/cmake/GTest )
    add_subdirectory( test/ctest )
    add_subdirectory( test/gtest )
endif()

# sub directories
# ===============
add_subdirectory( src )

# CPack
# =====
if( BUILD_PACKAGES )
    include( doc/docstrings.cmake )
    set( CPACK_STRIP_FILES true )
    set( CPACK_GENERATOR "DEB" )
    set( CPACK_DEBIAN_PACKAGE_VERSION ${DOC_VERSION} )
    set( CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libopenimageio1.3" )
    set( CPACK_DEBIAN_PACKAGE_MAINTAINER ${DOC_AUTHORS} )
    string( REPLACE "\n" "\n " CPACK_DEBIAN_PACKAGE_DESCRIPTION ${DOC_DESCRIPTION_SHORT} "\n" ${DOC_DESCRIPTION} )
    set( CPACK_DEBIAN_PACKAGE_SECTION "utils" )
    include( CPack )
endif()
