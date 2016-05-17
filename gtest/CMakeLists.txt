include_directories( ${CMAKE_SOURCE_DIR}/src )

add_executable( gtests test.cpp )
target_link_libraries( gtests
    smf_tools ${GTEST_LIBS} ${LIBS} )

add_executable( channel_test channel_test.cpp )
target_link_libraries( channel_test ${OPENIMAGEIO_LIBRARY} )
