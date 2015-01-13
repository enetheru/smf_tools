#!/bin/bash
COMMAND="valgrind --leak-check=full echo not yet implemented"
echo COMMAND = $COMMAND
eval $COMMAND
exit 1 #$?