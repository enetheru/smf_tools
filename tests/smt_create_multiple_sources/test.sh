#!/bin/bash
COMMAND="smt_decc -v --smt ../data/data.smt ../data/image_1.png"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
