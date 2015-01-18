#!/bin/bash
COMMAND="smt_info ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
