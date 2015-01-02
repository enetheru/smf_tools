#!/bin/bash
COMMAND="smt_decc -v ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
