
# == Help Tests ==
add_test( NAME smt_repair COMMAND $<TARGET_FILE:smt_repair> )
set_tests_properties( smt_repair PROPERTIES LABELS "smt_repair" WILL_FAIL TRUE )

add_test( NAME smt_repair_h COMMAND $<TARGET_FILE:smt_repair> -h )
set_tests_properties( smt_repair_h PROPERTIES LABELS "smt_repair help" )

add_test( NAME smt_repair_help COMMAND $<TARGET_FILE:smt_repair> --help )
set_tests_properties( smt_repair_help PROPERTIES LABELS "smt_repair help" )

add_test( NAME smt_repair_version COMMAND $<TARGET_FILE:smt_repair> --version )
set_tests_properties( smt_repair_version PROPERTIES LABELS "smt_repair version" )

# == Other Tests ==