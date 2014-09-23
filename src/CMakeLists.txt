configure_file( config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_library( tilemap tilemap.cpp )
add_library( tilecache tilecache.cpp )
add_library( smf smf.cpp )
add_library( smt smt.cpp smtool.cpp )
add_library( tiledimage tiledimage.cpp )
add_library( util util.cpp )

add_executable( smf_cc smf_cc.cpp)
target_link_libraries( smf_cc
    tilecache
    tilemap
    smf
    smt
    util
    ${LIBS} )

add_executable( smt_cc smt_cc.cpp )
target_link_libraries( smt_cc
    tilecache
    tilemap
    tiledimage
    smf
    smt
    util
    ${LIBS} )

add_executable( smf_decc smf_decc.cpp )
target_link_libraries( smf_decc 
    smf
    smt
    util
    tilemap
    ${LIBS} )

add_executable( smt_decc smt_decc.cpp )
target_link_libraries( smt_decc
    smt
    util
    ${LIBS} )

add_executable( smt_info smt_info.cpp )
target_link_libraries( smt_info
   smt
   util
   ${LIBS} )

add_executable( smf_info smf_info.cpp )
target_link_libraries( smf_info
   smf
   smt
   util
   tilemap
   ${LIBS} )

INSTALL(PROGRAMS
	${CMAKE_CURRENT_BINARY_DIR}/smf_cc
	${CMAKE_CURRENT_BINARY_DIR}/smf_decc
	${CMAKE_CURRENT_BINARY_DIR}/smf_info
	${CMAKE_CURRENT_BINARY_DIR}/smt_cc
	${CMAKE_CURRENT_BINARY_DIR}/smt_decc
	${CMAKE_CURRENT_BINARY_DIR}/smt_info
       	DESTINATION bin)
