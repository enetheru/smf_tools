#!/bin/bash
COMMAND="smf_cc -f test.smf"
echo $COMMAND
eval $COMMAND
exit $?