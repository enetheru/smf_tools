add_test( NAME smf_decc COMMAND $<TARGET_FILE:smf_decc> )
set_tests_properties( smf_decc PROPERTIES LABELS "smf_decc" WILL_FAIL TRUE )

add_test( NAME smf_decc_h COMMAND $<TARGET_FILE:smf_decc> -h )
set_tests_properties( smf_decc_h PROPERTIES LABELS "smf_decc help")

add_test( NAME smf_decc_help COMMAND $<TARGET_FILE:smf_decc> -help )
set_tests_properties( smf_decc_help PROPERTIES LABELS "smf_decc help")

add_test( NAME smf_decc_extract COMMAND $<TARGET_FILE:smf_decc> ${TEST_DATA_DIR}/data.smf )
set_tests_properties( smf_decc_extract PROPERTIES LABELS "smf_decc")
