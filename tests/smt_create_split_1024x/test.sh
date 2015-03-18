#!/bin/bash
oiiotool --resample 1024x1024 ../data/image_1.png -o test.jpg
COMMAND="smt_convert -vf --smt --tilesize 32x32 ./test.jpg"
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND
smt_info output.smt

exit $?
