#!/bin/bash
COMMAND="smf_info ../data/data.smf"
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND
exit $?
