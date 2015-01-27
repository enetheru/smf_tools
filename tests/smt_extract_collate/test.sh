#!/bin/bash
COMMAND="smt_convert -v --img ../data/data.smt"
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND
exit $?
