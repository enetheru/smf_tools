configure_file( config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_library( tilemap tilemap.cpp )
add_library( tilecache tilecache.cpp )
add_library( smf smf.cpp )
add_library( smt smt.cpp )

add_executable( mapconv
	mapconv.cpp
    util.cpp )

target_link_libraries( mapconv
    tilecache
    tilemap
    smf
    smt
    ${LIBS} )

add_executable( tileconv
	tileconv.cpp
    smtool.cpp
    util.cpp )

target_link_libraries( tileconv
    tilecache
    tilemap
    smf
    smt
    ${LIBS} )

INSTALL(PROGRAMS
	${CMAKE_CURRENT_BINARY_DIR}/mapconv
	${CMAKE_CURRENT_BINARY_DIR}/tileconv
       	DESTINATION bin)
