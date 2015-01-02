#!/bin/bash
COMMAND="smt_decc -v -c ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?