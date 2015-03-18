#!/bin/bash
COMMAND="smt_convert -vf --smt --imagesize 1024x1024 --tilesize 64x64 ../data/data.smt ../data/image_1.png"
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND
exit $?
