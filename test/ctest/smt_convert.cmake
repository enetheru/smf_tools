add_test( NAME smt_convert COMMAND $<TARGET_FILE:smt_convert> )
set_tests_properties( smt_convert PROPERTIES LABELS "smt_convert" WILL_FAIL TRUE )

file(GLOB IMG_FILES ${TEST_DATA_DIR}/tiles/*.BMP)
add_test( NAME smt_convert_tiles COMMAND $<TARGET_FILE:smt_convert> -v --smt ${IMG_FILES} )
set_tests_properties( smt_convert_tiles PROPERTIES LABELS "smt_convert" )

add_test( NAME smt_convert_template COMMAND $<TARGET_FILE:smt_convert> -v --smt --tilesize 32x32 --imagesize 1024x1024 ${TEST_DATA_DIR}/image_1.png )
set_tests_properties( smt_convert_template PROPERTIES LABELS "smt_convert" )

add_test( NAME smt_convert_smt COMMAND $<TARGET_FILE:smt_convert> -vf --smt ${TEST_DATA_DIR}/data.smt )
set_tests_properties( smt_convert_smt PROPERTIES LABELS "smt_convert" )

add_test( NAME smt_convert_smt_img COMMAND $<TARGET_FILE:smt_convert> -v --img --tilemap ${TEST_DATA_DIR}/data.smf ${TEST_DATA_DIR}/data.smt )
set_tests_properties( smt_convert_smt_img PROPERTIES LABELS "smt_convert" )

add_test( NAME smt_convert_smf_split128x COMMAND $<TARGET_FILE:smt_convert> -v --tilesize 128x128 --img --tilemap ${TEST_DATA_DIR}/data.smf ${TEST_DATA_DIR}/data.smt )
set_tests_properties( smt_convert_smf_split128x PROPERTIES LABELS "smt_convert" )