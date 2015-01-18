#!/bin/bash
COMMAND="smt_decc -v --img --imagesize 8192x1024 --tilesize 128x128 --tilemap ../data/data.smf ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
