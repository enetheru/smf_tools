set( TEST_DATA ${CMAKE_SOURCE_DIR}/test/data)

# smf_info
# ========
add_test( NAME smf_info COMMAND $<TARGET_FILE:smf_info> )
set_tests_properties( smf_info PROPERTIES LABELS "smf_info" WILL_FAIL TRUE )

add_test( NAME smf_info_h COMMAND $<TARGET_FILE:smf_info> -h )
set_tests_properties( smf_info_h PROPERTIES LABELS "smf_info help" )

add_test( NAME smf_info_help COMMAND $<TARGET_FILE:smf_info> --help )
set_tests_properties( smf_info_help PROPERTIES LABELS "smf_info help" )

add_test( NAME smf_info_default COMMAND $<TARGET_FILE:smf_info> ${TEST_DATA}/data.smf )
set_tests_properties( smf_info_default PROPERTIES LABELS "smf_info" )

# smf_cc
# ======
add_test( NAME smf_cc COMMAND $<TARGET_FILE:smf_cc> )
set_tests_properties( smf_cc PROPERTIES LABELS "smf_cc" WILL_FAIL TRUE )

add_test( NAME smf_cc_h COMMAND $<TARGET_FILE:smf_cc> -h )
set_tests_properties( smf_cc_h PROPERTIES LABELS "smf_cc help" )

add_test( NAME smf_cc_help COMMAND $<TARGET_FILE:smf_cc> -help )
set_tests_properties( smf_cc_help PROPERTIES LABELS "smf_cc help" )

# smf_decc
# ========
add_test( NAME smf_decc COMMAND $<TARGET_FILE:smf_decc> )
set_tests_properties( smf_decc PROPERTIES LABELS "smf_decc" WILL_FAIL TRUE )

add_test( NAME smf_decc_h COMMAND $<TARGET_FILE:smf_decc> -h )
set_tests_properties( smf_decc_h PROPERTIES LABELS "smf_decc help")

add_test( NAME smf_decc_help COMMAND $<TARGET_FILE:smf_decc> -help )
set_tests_properties( smf_decc_help PROPERTIES LABELS "smf_decc help")

add_test( NAME smf_decc_extract COMMAND $<TARGET_FILE:smf_decc> ${TEST_DATA}/data.smf )
set_tests_properties( smf_decc_extract PROPERTIES LABELS "smf_decc")

# smt_info
# ========
add_test( NAME smt_info COMMAND $<TARGET_FILE:smt_info> )
set_tests_properties( smt_info PROPERTIES LABELS "smt_info" WILL_FAIL TRUE )

add_test( NAME smt_info_h COMMAND $<TARGET_FILE:smt_info> -h )
set_tests_properties( smt_info_h PROPERTIES LABELS "smt_info help" )

add_test( NAME smt_info_help COMMAND $<TARGET_FILE:smt_info> --help )
set_tests_properties( smt_info_help PROPERTIES LABELS "smt_info help" )


# smt_convert
# ===========
add_test( NAME smt_convert COMMAND $<TARGET_FILE:smt_convert> )
set_tests_properties( smt_convert PROPERTIES LABELS "smt_convert" WILL_FAIL TRUE )

file(GLOB IMG_FILES ${TEST_DATA}/tiles/*.BMP)
add_test( NAME smt_convert_tiles COMMAND $<TARGET_FILE:smt_convert> -v --smt ${IMG_FILES} )
set_tests_properties( smt_convert_tiles PROPERTIES LABELS "smt_convert" )

add_test( NAME smt_convert_template COMMAND $<TARGET_FILE:smt_convert> -v --smt --tilesize 32x32 --imagesize 1024x1024 ${TEST_DATA}/image_1.png )
set_tests_properties( smt_convert_template PROPERTIES LABELS "smt_convert" )

add_test( NAME smt_convert_smt COMMAND $<TARGET_FILE:smt_convert> -vf --smt ${TEST_DATA}/data.smt )
set_tests_properties( smt_convert_smt PROPERTIES LABELS "smt_convert" )

add_test( NAME smt_convert_smt_img COMMAND $<TARGET_FILE:smt_convert> -v --img --tilemap ${TEST_DATA}/data.smf ${TEST_DATA}/data.smt )
set_tests_properties( smt_convert_smt_img PROPERTIES LABELS "smt_convert" )

add_test( NAME smt_convert_smf_split128x COMMAND $<TARGET_FILE:smt_convert> -v --tilesize 128x128 --img --tilemap ${TEST_DATA}/data.smf ${TEST_DATA}/data.smt )
set_tests_properties( smt_convert_smf_split128x PROPERTIES LABELS "smt_convert" )

# smt_repair
# ==========
add_test( NAME smt_repair  COMMAND $<TARGET_FILE:smt_repair> )
set_tests_properties( smt_repair PROPERTIES LABELS "smt_repair" WILL_FAIL TRUE )


