add_test( NAME smf_cc COMMAND $<TARGET_FILE:smf_cc> )
set_tests_properties( smf_cc PROPERTIES LABELS "smf_cc" WILL_FAIL TRUE )

add_test( NAME smf_cc_h COMMAND $<TARGET_FILE:smf_cc> -h )
set_tests_properties( smf_cc_h PROPERTIES LABELS "smf_cc help" )

add_test( NAME smf_cc_help COMMAND $<TARGET_FILE:smf_cc> -help )
set_tests_properties( smf_cc_help PROPERTIES LABELS "smf_cc help" )