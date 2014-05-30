add_executable( mapconv
	mapconv.cpp
	nvtt_output_handler.cpp nvtt_output_handler.h
	dxt1load.cpp dxt1load.h
	smf.cpp smf.h
	smt.cpp smt.h )

target_link_libraries( mapconv ${LIBS} )

add_executable( tileconv
	tileconv.cpp
	nvtt_output_handler.cpp nvtt_output_handler.h
	dxt1load.cpp dxt1load.h
	smf.cpp smf.h
	smt.cpp smt.h )

target_link_libraries( tileconv ${LIBS} )

INSTALL(PROGRAMS
	${CMAKE_CURRENT_BINARY_DIR}/mapconv
	${CMAKE_CURRENT_BINARY_DIR}/tileconv
       	DESTINATION bin)