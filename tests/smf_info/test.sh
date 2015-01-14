#!/bin/bash
COMMAND="valgrind --leak-check=full --suppressions=../suppressions.supp smf_info ../data/data.smf"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?