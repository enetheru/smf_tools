#!/bin/bash
COMMAND="smt_decc -v --img --tilemap ../data/data.smf ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
