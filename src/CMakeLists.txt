configure_file( config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_library( tilemap tilemap.cpp )
add_library( tilecache tilecache.cpp )
add_library( smf smf.cpp )
add_library( smt smt.cpp smtool.cpp )
add_library( tiledimage tiledimage.cpp )
add_library( util util.cpp )

add_executable( mapconv mapconv.cpp)
target_link_libraries( mapconv
    tilecache
    tilemap
    smf
    smt
    util
    ${LIBS} )

add_executable( tileconv tileconv.cpp )
target_link_libraries( tileconv
    tilecache
    tilemap
    tiledimage
    smf
    smt
    util
    ${LIBS} )


add_executable( smtinfo smtinfo.cpp )
target_link_libraries( smtinfo
   smt
   util
   ${LIBS} )

add_executable( smfinfo smfinfo.cpp )
target_link_libraries( smfinfo
   smf
   smt
   util
   tilemap
   ${LIBS} )

INSTALL(PROGRAMS
	${CMAKE_CURRENT_BINARY_DIR}/mapconv
	${CMAKE_CURRENT_BINARY_DIR}/tileconv
	${CMAKE_CURRENT_BINARY_DIR}/smtinfo
	${CMAKE_CURRENT_BINARY_DIR}/smtextract
       	DESTINATION bin)
