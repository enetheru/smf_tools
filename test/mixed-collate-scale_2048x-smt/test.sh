#!/bin/bash
COMMAND="smt_convert -vf --smt --imagesize 2048x2048 ../data/data.smt ../data/image_1.png ../data/tiles/*"
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND
exit $?
