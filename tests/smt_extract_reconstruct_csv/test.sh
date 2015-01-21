#!/bin/bash
COMMAND="smt_convert -v --img --tilemap ../data/tilemap.csv ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
