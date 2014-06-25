add_executable( mapconv
    config.h
	mapconv.cpp
	nvtt_output_handler.cpp nvtt_output_handler.h
	dxt1load.cpp dxt1load.h
	smf.cpp smf.h
	smt.cpp smt.h
    util.cpp util.h
    tilecache.cpp tilecache.h )

target_link_libraries( mapconv ${LIBS} )

add_executable( tileconv
    config.h
	tileconv.cpp
	nvtt_output_handler.cpp nvtt_output_handler.h
	dxt1load.cpp dxt1load.h
	smf.cpp smf.h
	smt.cpp smt.h
    smtool.cpp smtool.h
    smf_util.h smf_util.cpp
    util.cpp util.h
    tilecache.cpp tilecache.h)

target_link_libraries( tileconv ${LIBS} )

INSTALL(PROGRAMS
	${CMAKE_CURRENT_BINARY_DIR}/mapconv
	${CMAKE_CURRENT_BINARY_DIR}/tileconv
       	DESTINATION bin)
