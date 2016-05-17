include_directories( ${CMAKE_SOURCE_DIR}/src )

add_executable( gtests test.cpp )
target_link_libraries( gtests
    smf_tools ${GTEST_LIBS} ${LIBS} )
