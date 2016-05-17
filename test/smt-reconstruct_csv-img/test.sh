#!/bin/bash
COMMAND="smt_convert -v --img --tilemap ../data/tilemap.csv ../data/data.smt"

# == Pre Test Actions ==
#nothing

# == Test Action ==
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND

# == Post test actions ==
#nothing
exit $?
