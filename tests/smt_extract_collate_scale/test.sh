#!/bin/bash
COMMAND="smt_decc -v --imagesize 512x512 --img ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
