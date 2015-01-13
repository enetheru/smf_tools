#!/bin/bash
COMMAND="valgrind --leak-check=full smf_info ../data/data.smf"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?