#!/bin/bash
COMMAND="smt_convert -v --tilesize 128x128 --img ../data/data.smt"
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND
exit $?
