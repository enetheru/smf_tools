#!/bin/bash
COMMAND="valgrind --leak-check=full --suppressions=../suppressions.supp smt_info ../data/data.smt"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?