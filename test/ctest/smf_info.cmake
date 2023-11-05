
# == Help Tests ==
add_test( NAME smf_info COMMAND $<TARGET_FILE:smf_info> )
set_tests_properties( smf_info PROPERTIES LABELS "smf_info" WILL_FAIL TRUE )

add_test( NAME smf_info_unknown COMMAND $<TARGET_FILE:smt_info> --beans )
set_tests_properties( smf_info_unknown PROPERTIES LABELS "smf_info" WILL_FAIL TRUE )

add_test( NAME smf_info_h COMMAND $<TARGET_FILE:smf_info> -h )
set_tests_properties( smf_info_h PROPERTIES LABELS "smf_info help" )

add_test( NAME smf_info_help COMMAND $<TARGET_FILE:smf_info> --help )
set_tests_properties( smf_info_help PROPERTIES LABELS "smf_info help" )

add_test( NAME smf_info_version COMMAND $<TARGET_FILE:smf_info> --version )
set_tests_properties( smf_info_version PROPERTIES LABELS "smf_info version" )

# == Other Tests ==

add_test( NAME smf_info_data COMMAND $<TARGET_FILE:smf_info> ${TEST_DATA_DIR}/data.smf )
set_tests_properties( smf_info_data PROPERTIES LABELS "smf_info" )

add_test( NAME smf_info_data2 COMMAND $<TARGET_FILE:smf_info> ${TEST_DATA_DIR}/data.smf ${TEST_DATA_DIR}/data.smf )
set_tests_properties( smf_info_data2 PROPERTIES LABELS "smf_info" )