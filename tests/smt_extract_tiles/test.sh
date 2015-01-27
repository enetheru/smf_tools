#!/bin/bash
COMMAND="smt_convert -v --img --tilesize 32x32 ../data/data.smt"
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND
exit $?
