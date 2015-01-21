#!/bin/bash
COMMAND="smf_decc -v ../data/data.smf"
echo COMMAND = $COMMAND
eval $COMMAND
exit $?
