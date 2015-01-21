#!/bin/bash
COMMAND="smt_convert --img --imagesize 16384x16384 --tilemap ../data/data.smf ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
