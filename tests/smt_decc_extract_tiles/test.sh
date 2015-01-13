#!/bin/bash
COMMAND="valgrind --leak-check=full smt_decc -v ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
