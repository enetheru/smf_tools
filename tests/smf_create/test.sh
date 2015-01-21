#!/bin/bash
COMMAND="smf_cc -v"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
