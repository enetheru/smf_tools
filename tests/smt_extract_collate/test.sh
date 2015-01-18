#!/bin/bash
COMMAND="smt_decc -v --img ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
