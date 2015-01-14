#!/bin/bash
COMMAND="valgrind --leak-check=full --suppressions=../suppressions.supp smt_decc -v --smt ../data/data.smt ../data/image_1.png"
echo COMMAND = $COMMAND
eval $COMMAND
exit 1 $?