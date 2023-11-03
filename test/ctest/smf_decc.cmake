# == Help Tests ==
add_test( NAME smf_decc COMMAND $<TARGET_FILE:smf_decc> )
set_tests_properties( smf_decc PROPERTIES LABELS "smf_decc" WILL_FAIL TRUE )

add_test( NAME smf_decc_h COMMAND $<TARGET_FILE:smf_decc> -h )
set_tests_properties( smf_decc_h PROPERTIES LABELS "smf_decc help" )

add_test( NAME smf_decc_help COMMAND $<TARGET_FILE:smf_decc> --help )
set_tests_properties( smf_decc_help PROPERTIES LABELS "smf_decc help" )

add_test( NAME smf_decc_version COMMAND $<TARGET_FILE:smf_decc> --version )
set_tests_properties( smf_decc_version PROPERTIES LABELS "smf_decc version" )

# == Other Tests ==
add_test( NAME smf_decc_extract COMMAND $<TARGET_FILE:smf_decc> -v ${TEST_DATA_DIR}/data.smf )
set_tests_properties( smf_decc_extract PROPERTIES LABELS "smf_decc")

#[[ == Testing Output Artifacts ==
    Since decompilation spits out files, I need a way to test the output artifacts.
    This would require helper tools to make that happen. I'm thinking file checksums,
    etc, and I need these tools to be universally available on windows and linux so
    that the test can run regardless of the platform

    Options for checking
    * File Checksum
    * Image Checksum
    * File Content
    * File Exists
]]
# TODO: Testing output artifact: Header Info
add_test( NAME smf_decc-header COMMAND $<TARGET_FILE:smf_decc> -v )
set_property( TEST smf_decc-header PROPERTY LABELS "smf_decc")
set_property( TEST smf_decc-header PROPERTY DEPENDS "smf_decc_extract")

# TODO: Testing output artifact: height image
add_test( NAME smf_decc-height COMMAND $<TARGET_FILE:smf_decc> -v )
set_property( TEST smf_decc-height PROPERTY LABELS "smf_decc")
set_property( TEST smf_decc-height PROPERTY DEPENDS "smf_decc_extract")

# TODO: Testing output artifact: type image
add_test( NAME smf_decc-type COMMAND $<TARGET_FILE:smf_decc> -v )
set_property( TEST smf_decc-type PROPERTY LABELS "smf_decc")
set_property( TEST smf_decc-type PROPERTY DEPENDS "smf_decc_extract")

# TODO: Testing output artifact: map image
add_test( NAME smf_decc-map COMMAND $<TARGET_FILE:smf_decc> -v )
set_property( TEST smf_decc-map PROPERTY LABELS "smf_decc")
set_property( TEST smf_decc-map PROPERTY DEPENDS "smf_decc_extract")

# TODO: Testing output artifact: mini image
add_test( NAME smf_decc-mini COMMAND $<TARGET_FILE:smf_decc> -v )
set_property( TEST smf_decc-mini PROPERTY LABELS "smf_decc")
set_property( TEST smf_decc-mini PROPERTY DEPENDS "smf_decc_extract")

# TODO: Testing output artifact: metal image
add_test( NAME smf_decc-metal COMMAND $<TARGET_FILE:smf_decc> -v )
set_property( TEST smf_decc-metal PROPERTY LABELS "smf_decc")
set_property( TEST smf_decc-metal PROPERTY DEPENDS "smf_decc_extract")

# TODO: Testing output artifact: featureList
add_test( NAME smf_decc-featurelist COMMAND $<TARGET_FILE:smf_decc> -v )
set_property( TEST smf_decc-featurelist PROPERTY LABELS "smf_decc")
set_property( TEST smf_decc-featurelist PROPERTY DEPENDS "smf_decc_extract")

# TODO: Testing output artifact: features
add_test( NAME smf_decc-features COMMAND $<TARGET_FILE:smf_decc> -v )
set_property( TEST smf_decc-features PROPERTY LABELS "smf_decc")
set_property( TEST smf_decc-features PROPERTY DEPENDS "smf_decc_extract")

# TODO: Testing output artifact: grass image
add_test( NAME smf_decc-grass COMMAND $<TARGET_FILE:smf_decc> -v )
set_property( TEST smf_decc-grass PROPERTY LABELS "smf_decc")
set_property( TEST smf_decc-grass PROPERTY DEPENDS "smf_decc_extract")


