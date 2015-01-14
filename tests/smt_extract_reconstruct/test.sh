#!/bin/bash
COMMAND="valgrind --leak-check=full --suppressions=../suppressions.supp smt_decc -v --tilemap ../data.smf --img ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?