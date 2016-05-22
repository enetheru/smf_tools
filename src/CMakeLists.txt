configure_file( smf_tools.h.in ${CMAKE_CURRENT_BINARY_DIR}/smf_tools.h )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_library( smf_tools
    smf.cpp         smf.h
    smt.cpp         smt.h
    tilemap.cpp     tilemap.h
    tilecache.cpp   tilecache.h
    tiledimage.cpp  tiledimage.h
    util.cpp        util.h )

target_link_libraries( smf_tools ${LIBS} )

add_executable( smf_cc smf_cc.cpp )
target_link_libraries( smf_cc smf_tools ${LIBS} )

add_executable( smf_decc smf_decc.cpp )
target_link_libraries( smf_decc smf_tools ${LIBS} )

add_executable( smt_convert smt_convert.cpp )
target_link_libraries( smt_convert smf_tools ${LIBS} )

add_executable( smt_info smt_info.cpp )
target_link_libraries( smt_info smf_tools ${LIBS} )

add_executable( smf_info smf_info.cpp )
target_link_libraries( smf_info smf_tools ${LIBS} )

add_executable( smt_repair smt_repair.cpp )
target_link_libraries( smt_repair smf_tools ${LIBS} )

INSTALL(PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/smf_cc
    ${CMAKE_CURRENT_BINARY_DIR}/smf_decc
    ${CMAKE_CURRENT_BINARY_DIR}/smf_info
    ${CMAKE_CURRENT_BINARY_DIR}/smt_convert
    ${CMAKE_CURRENT_BINARY_DIR}/smt_info
    ${CMAKE_CURRENT_BINARY_DIR}/smt_repair
    ${CMAKE_CURRENT_SOURCE_DIR}/makemap.sh
           DESTINATION bin)
