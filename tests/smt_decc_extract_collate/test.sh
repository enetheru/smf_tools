#!/bin/bash
COMMAND="valgrind --leak-check=full smt_decc -v -c ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?