#!/bin/bash
COMMAND="valgrind --leak-check=full smt_cc -v -f smt.smt ../data/image_1.png"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?