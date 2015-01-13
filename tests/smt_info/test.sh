#!/bin/bash
COMMAND="valgrind --leak-check=full smt_info ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?