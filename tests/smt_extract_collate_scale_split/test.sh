#!/bin/bash
COMMAND="smt_decc -v --imagesize 1024x1024 --tilesize 128x128 --img ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
