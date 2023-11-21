add_subdirectory( smflib )

add_executable( smf_cc smf_cc.cpp )
target_include_directories( smf_cc PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smf_cc smflib -static )
target_link_options( smf_cc PUBLIC "LINKER:--allow-multiple-definition")

add_executable( smf_decc smf_decc.cpp )
target_include_directories( smf_decc PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smf_decc smflib -static )
target_link_options( smf_decc PUBLIC "LINKER:--allow-multiple-definition")

add_executable( smt_convert smt_convert.cpp )
target_include_directories( smt_convert PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smt_convert smflib -static )
target_link_options( smt_convert PUBLIC "LINKER:--allow-multiple-definition")

add_executable( smt_info smt_info.cpp )
target_include_directories( smt_info PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smt_info smflib -static )
target_link_options( smt_info PUBLIC "LINKER:--allow-multiple-definition")

add_executable( smf_info smf_info.cpp )
target_include_directories( smf_info PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smf_info smflib -static )
target_link_options( smf_info PUBLIC "LINKER:--allow-multiple-definition")

#add_executable(mapconv mapconv.cpp)
#target_include_directories(mapconv PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
#target_link_libraries(mapconv smflib -static)
#target_link_options(mapconv PUBLIC "LINKER:--allow-multiple-definition")

add_executable( mapconv mapconv.cpp )
target_include_directories( mapconv PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( mapconv smflib -static )
target_link_options( mapconv PUBLIC "LINKER:--allow-multiple-definition")

add_executable( emapconv
        emapconv.cpp
        emapconv_args.h)
target_include_directories( emapconv PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser ${TCLAP_INCLUDES} )
target_link_libraries( emapconv smflib -static )
target_link_options( emapconv PUBLIC "LINKER:--allow-multiple-definition")


INSTALL(
        PROGRAMS
        ${CMAKE_CURRENT_BINARY_DIR}/smf_cc
        ${CMAKE_CURRENT_BINARY_DIR}/smf_decc
        ${CMAKE_CURRENT_BINARY_DIR}/smf_info
        ${CMAKE_CURRENT_BINARY_DIR}/smt_convert
        ${CMAKE_CURRENT_BINARY_DIR}/smt_info
        ${CMAKE_CURRENT_BINARY_DIR}/smt_repair
        ${CMAKE_CURRENT_SOURCE_DIR}/makemap.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/mapconv
        ${CMAKE_CURRENT_SOURCE_DIR}/emapconv
        DESTINATION bin)
