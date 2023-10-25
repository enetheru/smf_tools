include_directories( ${CMAKE_SOURCE_DIR}/src )

add_executable( gtests test.cpp
        squish.cpp
        tilemap.cpp
        tilecache.cpp)

target_link_libraries( gtests
        PRIVATE
                -static
        PUBLIC
                smf_tools
                GTest::gtest_main
                libsquish::Squish
        )
