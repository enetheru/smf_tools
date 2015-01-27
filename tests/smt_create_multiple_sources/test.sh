#!/bin/bash
COMMAND="smt_convert -v --smt ../data/data.smt ../data/image_1.png"
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND
exit $?
