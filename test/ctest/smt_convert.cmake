
# == Help Tests ==
add_test( NAME smt_convert COMMAND $<TARGET_FILE:smt_convert> )
set_tests_properties( smt_convert PROPERTIES LABELS "smt_convert" WILL_FAIL TRUE )

add_test( NAME smt_convert_h COMMAND $<TARGET_FILE:smt_convert> -h )
set_tests_properties( smt_convert_h PROPERTIES LABELS "smt_convert help" )

add_test( NAME smt_convert_help COMMAND $<TARGET_FILE:smt_convert> --help )
set_tests_properties( smt_convert_help PROPERTIES LABELS "smt_convert help" )

add_test( NAME smt_convert_version COMMAND $<TARGET_FILE:smt_convert> --version )
set_tests_properties( smt_convert_version PROPERTIES LABELS "smt_convert version" )

# == Other Tests ==

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

# commands from testing shell scripts:
# COMMAND="smt_convert -vf --smt --tilesize 32x32 --imagesize 1024x1024 ../data/image_1.png"

# COMMAND="smt_convert -vf --smt ../data/image_1.png"
# POST: smt_convert -vf --img --tilemap output.smt.csv output.smt

# PRE: oiiotool --resample 1024x1024 ../data/image_1.png -o test.jpg
# COMMAND="smt_convert -vf --smt ./test.jpg"

# COMMAND="smt_convert -vf --smt --imagesize 2048x2048 ../data/data.smt ../data/image_1.png ../data/tiles/*"

# COMMAND="smt_convert -vf --imagesize 1024x1024 --img ../data/data.smt"

# COMMAND="smt_convert -vf --img ../data/data.smt"

# COMMAND="smt_convert -v --imagesize 1024x1024 --tilesize 128x128 --img ../data/data.smt"

# COMMAND="smt_convert -v --img --tilesize 32x32 ../data/data.smt"

# COMMAND="smt_convert -v --tilesize 128x128 --img ../data/data.smt"

# COMMAND="smt_convert -v --img --tilemap ../data/tilemap.csv ../data/data.smt"

# COMMAND="smt_convert --img --imagesize 512x1024 --tilesize 128x128 --tilemap ../data/data.smf ../data/data.smt"

# COMMAND="smt_convert -v --imagesize 512x512 --img --tilemap ../data/data.smf ../data/data.smt"

# COMMAND="smt_convert --img --imagesize 1024x512 --tilesize 128x128 --tilemap ../data/data.smf ../data/data.smt"

# COMMAND="smt_convert -v --img --imagesize 8192x1024 --tilesize 128x128 --tilemap ../data/data.smf ../data/data.smt"