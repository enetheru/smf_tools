#!/bin/bash
COMMAND="smf_decc -v ../data/data.smf"

# == Pre Test Actions ==
#nothing

# == Test Action ==
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND

# == Post test actions ==
#nothing
exit $?
