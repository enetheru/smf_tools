include_directories( ${CMAKE_SOURCE_DIR}/src )

add_compile_definitions("-DSOURCE_ROOT=\"${CMAKE_SOURCE_DIR}\"")

add_executable( gtests main.cpp
        t_squish.cpp
        t_tilemap.cpp
        t_tilecache.cpp
        t_filemap.cpp
        t_util.cpp)

target_link_libraries( gtests
        PRIVATE
                -static
        PUBLIC
                smf_tools
                GTest::gtest_main
                libsquish::Squish
        )
