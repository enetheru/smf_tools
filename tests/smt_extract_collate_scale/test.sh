#!/bin/bash
COMMAND="smt_convert -v --imagesize 512x512 --img ../data/data.smt"
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND
exit $?
