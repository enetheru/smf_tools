#!/bin/bash
COMMAND="smt_convert --img --imagesize 512x1024 --tilesize 128x128 --tilemap ../data/data.smf ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
