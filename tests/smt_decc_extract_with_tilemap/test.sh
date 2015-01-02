#!/bin/bash
COMMAND="smt_decc -v --tilemap ../data.smf ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?