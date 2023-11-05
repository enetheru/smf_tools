
# == Help Tests ==
add_test( NAME smt_info COMMAND $<TARGET_FILE:smt_info> )
set_tests_properties( smt_info PROPERTIES LABELS "smt_info" WILL_FAIL TRUE )

add_test( NAME smt_info_unknown COMMAND $<TARGET_FILE:smt_info> --beans )
set_tests_properties( smt_info_unknown PROPERTIES LABELS "smt_info" WILL_FAIL TRUE )

add_test( NAME smt_info_h COMMAND $<TARGET_FILE:smt_info> -h )
set_tests_properties( smt_info_h PROPERTIES LABELS "smt_info help" )

add_test( NAME smt_info_help COMMAND $<TARGET_FILE:smt_info> --help )
set_tests_properties( smt_info_help PROPERTIES LABELS "smt_info help" )

add_test( NAME smt_info_version COMMAND $<TARGET_FILE:smt_info> --version )
set_tests_properties( smt_info_version PROPERTIES LABELS "smt_info version" )

# == Other Tests ==

add_test( NAME smt_info_data COMMAND $<TARGET_FILE:smt_info> ${TEST_DATA_DIR}/data.smt )
set_tests_properties( smt_info_data PROPERTIES LABELS "smt_info" )

add_test( NAME smt_info_data2 COMMAND $<TARGET_FILE:smt_info> ${TEST_DATA_DIR}/data.smt ${TEST_DATA_DIR}/data.smt )
set_tests_properties( smt_info_data2 PROPERTIES LABELS "smt_info" )
