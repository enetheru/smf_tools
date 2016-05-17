#!/bin/bash
COMMAND="smt_convert -vf --smt ../data/data.smt"

# == Pre Test Actions ==
#nothing

# == Test Action ==
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND

# == Post test actions ==
#nothing
exit $?
