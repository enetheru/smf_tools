add_test( NAME smt_info COMMAND $<TARGET_FILE:smt_info> )
set_tests_properties( smt_info PROPERTIES LABELS "smt_info" WILL_FAIL TRUE )

add_test( NAME smt_info_h COMMAND $<TARGET_FILE:smt_info> -h )
set_tests_properties( smt_info_h PROPERTIES LABELS "smt_info help" )

add_test( NAME smt_info_help COMMAND $<TARGET_FILE:smt_info> --help )
set_tests_properties( smt_info_help PROPERTIES LABELS "smt_info help" )

add_test( NAME smt_info_data COMMAND $<TARGET_FILE:smt_info> ${TEST_DATA_DIR}/data.smt )
set_tests_properties( smt_info_data PROPERTIES LABELS "smt_info" )
