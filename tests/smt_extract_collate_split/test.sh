#!/bin/bash
COMMAND="smt_decc -v --tilesize 128x128 --img ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
