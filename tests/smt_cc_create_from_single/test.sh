#!/bin/bash
COMMAND="smt_cc -v -f smt.smt ../data/image_1.png"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?