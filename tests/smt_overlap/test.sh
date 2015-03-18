#!/bin/bash
COMMAND="smt_convert -vf --img --type=RGBA8 --overlap 8 ../data/data.smt"
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND
exit $?
