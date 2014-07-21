configure_file( config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_executable( mapconv
	mapconv.cpp
	smf.cpp smf.h
	smt.cpp smt.h
    util.cpp util.h
    tilecache.cpp tilecache.h
    tilemap.cpp tilemap.h )

target_link_libraries( mapconv ${LIBS} )

add_executable( tileconv
	tileconv.cpp
	smf.cpp smf.h
	smt.cpp smt.h
    smtool.cpp smtool.h
    util.cpp util.h
    tilecache.cpp tilecache.h
    tilemap.cpp tilemap.h )

target_link_libraries( tileconv ${LIBS} )

INSTALL(PROGRAMS
	${CMAKE_CURRENT_BINARY_DIR}/mapconv
	${CMAKE_CURRENT_BINARY_DIR}/tileconv
       	DESTINATION bin)
