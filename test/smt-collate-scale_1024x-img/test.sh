#!/bin/bash
COMMAND="smt_convert -vf --imagesize 1024x1024 --img ../data/data.smt"
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND
exit $?
