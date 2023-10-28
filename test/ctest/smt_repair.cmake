add_test( NAME smt_repair  COMMAND $<TARGET_FILE:smt_repair> )
set_tests_properties( smt_repair PROPERTIES LABELS "smt_repair" WILL_FAIL TRUE )