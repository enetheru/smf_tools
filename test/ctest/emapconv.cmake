
# == Help Tests ==
add_test( NAME emapconv COMMAND $<TARGET_FILE:emapconv> )
set_tests_properties( emapconv PROPERTIES LABELS "emapconv" WILL_FAIL TRUE )

add_test( NAME emapconv_version COMMAND $<TARGET_FILE:emapconv> --version )
set_tests_properties( emapconv_version PROPERTIES LABELS "emapconv version" )

add_test( NAME emapconv_h COMMAND $<TARGET_FILE:emapconv> -h )
set_tests_properties( emapconv_h PROPERTIES LABELS "emapconv help" )

add_test( NAME emapconv_help COMMAND $<TARGET_FILE:emapconv> --help )
set_tests_properties( emapconv_help PROPERTIES LABELS "emapconv help" )

add_test( NAME emapconv_unknown COMMAND $<TARGET_FILE:emapconv> --beans )
set_tests_properties( emapconv_unknown PROPERTIES LABELS "emapconv unknown" WILL_FAIL TRUE )

add_test( NAME emapconv_nonoption COMMAND $<TARGET_FILE:emapconv> random.txt )
set_tests_properties( emapconv_nonoption PROPERTIES LABELS "emapconv nonoption" )

add_test( NAME emapconv_info COMMAND $<TARGET_FILE:emapconv>
        --mapx -1
        --mapy -1
        --mapz-min -5.0
        --mapz-max 1.0
        --map-id 5
        --square-size 13
        --texels-per-square 17
        --tile-size 128
        --compression-type 32
        ${TEST_DATA_DIR}/data.smf
        ${TEST_DATA_DIR}/data.smt
)
set_tests_properties( emapconv_info PROPERTIES LABELS "emapconv info" )

add_test( NAME emapconv_compile COMMAND $<TARGET_FILE:emapconv>
        -vvv
        --compile
        --mapx 256
        --mapy 256
        --mapz-min -128.0
        --mapz-max 128.0

        --height-map ${TEST_DATA_DIR}/height.tif
        --type-map ${TEST_DATA_DIR}/black.png
        --tile-map ${TEST_DATA_DIR}/black.png
        --mini-map ${TEST_DATA_DIR}/minimap.png
        --metal-map ${TEST_DATA_DIR}/metalmap.png
        --feature-map ${TEST_DATA_DIR}/black.png

        --diffuse-map ${TEST_DATA_DIR}/diffuse.png
)
set_tests_properties( emapconv_compile PROPERTIES LABELS "emapconv compile" )
