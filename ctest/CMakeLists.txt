# SMF based tests
add_test( smf_info_help0 ${CMAKE_BINARY_DIR}/src/smf_info )
add_test( smf_info_help1 ${CMAKE_BINARY_DIR}/src/smf_info -h )
add_test( smf_info_help2 ${CMAKE_BINARY_DIR}/src/smf_info --help )

# testing getting basic info from an smf file
add_test( smf_info
    ${CMAKE_BINARY_DIR}/src/smf_info ${CMAKE_CURRENT_SOURCE_DIR}/data/data.smf )


add_test( smf_cc_help   ${CMAKE_BINARY_DIR}/src/smf_cc )
add_test( smf_decc_help ${CMAKE_BINARY_DIR}/src/smf_decc )
add_test( smf_decc_extract
    ${CMAKE_BINARY_DIR}/src/smf_decc
    -v ${CMAKE_CURRENT_SOURCE_DIR}/data/data.smf )

# SMT based tests
add_test( smt_info_help    ${CMAKE_BINARY_DIR}/src/smt_info )
add_test( smt_convert_help ${CMAKE_BINARY_DIR}/src/smt_convert )
add_test( smt_repair_help  ${CMAKE_BINARY_DIR}/src/smt_repair )

