#!/bin/bash
COMMAND="echo command"
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND

exit $?