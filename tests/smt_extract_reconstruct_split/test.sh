#!/bin/bash
COMMAND="smt_convert -v --tilesize 128x128 --img --tilemap ../data/data.smf ../data/data.smt"
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND
exit $?
