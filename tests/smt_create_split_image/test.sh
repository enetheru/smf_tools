#!/bin/bash
COMMAND="smt_convert -vf --smt --tilesize 32x32 --imagesize 1024x1024 ../data/image_1.png"
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND
exit $?
