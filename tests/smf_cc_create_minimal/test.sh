#!/bin/bash
COMMAND="valgrind --leak-check=full smf_cc -f test.smf"
echo $COMMAND
eval $COMMAND
exit $?