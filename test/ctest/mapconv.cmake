
# == Help Tests ==
add_test( NAME mapconv COMMAND $<TARGET_FILE:mapconv> )
set_tests_properties( mapconv PROPERTIES LABELS "mapconv" WILL_FAIL TRUE )

add_test( NAME mapconv_h COMMAND $<TARGET_FILE:mapconv> -h )
set_tests_properties( mapconv_h PROPERTIES LABELS "mapconv help" )

add_test( NAME mapconv_help COMMAND $<TARGET_FILE:mapconv> --help )
set_tests_properties( mapconv_help PROPERTIES LABELS "mapconv help" )

add_test( NAME mapconv_version COMMAND $<TARGET_FILE:mapconv> --version )
set_tests_properties( mapconv_version PROPERTIES LABELS "mapconv version" )

# == Other Tests ==

#-k <feature placement file>,  --featureplacement <feature placement file>
#(value required)  A special text file defining the placement of each
#feature. (Default: fp.txt). See README.txt for details. The default
#format specifies it to have each line look like this (spacing is
#strict)
#
#{ name = 'agorm_talltree6', x = 224, z = 3616, rot = "0" }
#
#-j <feature list file>,  --featurelist <feature list file>
#(value required)  A file with the name of one feature on each line.
#(Default: fs.txt). See README.txt for details.
#
#Specifying a number from 32767 to -32768 next to the feature name will
#tell mapconv how much to rotate the feature. specifying -1 will rotate
#it randomly.
#
#Example line from fs.txt
#
#btreeblo_1 -1
#
#btreeblo 16000
#
#-f <featuremap image>,  --featuremap <featuremap image>
#(value required)  Feature placement file, xsize by ysize. See
#README.txt for details. Green 255 pixels are geo vents, blue is grass,
#green 201-215 is default trees, red 255-0 correspont each to a line in
#fs.txt.
#
#-r <randomrotate>,  --randomrotate <randomrotate>
#(value required)  rotate features randomly, the first r features in
#featurelist (fs.txt) get random rotation, default 0
#
#-s,  --justsmf
#Just create smf file, dont make smt
#
#-l,  --lowpass
#Lowpass filters the heightmap
#
#-q,  --use_nvcompress
#Use NVCOMPRESS.EXE tool for ultra fast CUDA compression. Needs Geforce
#8 or higher nvidia card.
#
#-i,  --invert
#Flip the height map image upside-down on reading.
#
#-z <texcompress program>,  --texcompress <texcompress program>
#(value required)  Name of companion program texcompress from current
#working directory.
#
#-c <compression>,  --compress <compression>
#(value required)  How much we should try to compress the texture map.
#Default 0.8, lower -> higher quality, larger files.
#
#-x <max height>,  --maxheight <max height>
#(required)  (value required)  What altitude in spring the max(0xff or
#0xffff) level of the height map represents.
#
#-n <min height>,  --minheight <min height>
#(required)  (value required)  What altitude in spring the min(0) level
#of the height map represents.
#
#-o <output .smf>,  --outfile <output .smf>
#(value required)  The name of the created map file. Should end in
#.smf. A tilefile (extension .smt) is also created.
#
#-e <tile file>,  --externaltilefile <tile file>
#(value required)  External tile file that will be used for finding
#tiles. Tiles not found in this will be saved in a new tile file.
#
#-g <Geovent image>,  --geoventfile <Geovent image>
#(value required)  The decal for geothermal vents; appears on the
#compiled map at each vent. (Default: geovent.bmp).
#
#-y <typemap image>,  --typemap <typemap image>
#(value required)  Type map to use, uses the red channel to decide
#type. types are defined in the .smd, if this argument is skipped the
#map will be all type 0
#
#-m <metalmap image>,  --metalmap <metalmap image>
#(required)  (value required)  Metal map to use, red channel is amount
#of metal. Resized to xsize / 2 by ysize / 2.
#
#-a <heightmap file>,  --heightmap <heightmap file>
#(required)  (value required)  Input heightmap to use for the map, this
#should be in 16 bit raw format (.raw extension) or an image file. Must
#be xsize*64+1 by ysize*64+1.
#
#-t <texturemap file>,  --intex <texturemap file>
#(required)  (value required)  Input bitmap to use for the map. Sides
#must be multiple of 1024 long. xsize, ysize determined from this file:
#xsize = intex width / 8, ysize = height / 8.
#
#--,  --ignore_rest
#Ignores the rest of the labeled arguments following this flag.
#
#-v,  --version
#Displays version information and exits.
#
#-h,  --help
#Displays usage information and exits.
#'