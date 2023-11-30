cmake_minimum_required(VERSION 3.24.0)

project(tclap)

find_package(spdlog REQUIRED HINTS C:/src/spdlog/cmake-build-debug/lib/cmake/spdlog )
find_package( fmt REQUIRED HINTS C:/src/fmt/cmake-build-debug/lib/cmake/fmt )

enable_testing()

add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/test )