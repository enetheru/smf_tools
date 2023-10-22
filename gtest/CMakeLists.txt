include_directories( ${CMAKE_SOURCE_DIR}/src )

add_executable( gtests test.cpp )
target_link_libraries( gtests
        PRIVATE
                -static
        PUBLIC
                smf_tools
                GTest::gtest_main
                libsquish::Squish
        )
