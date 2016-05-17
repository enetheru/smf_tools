#!/bin/bash
COMMAND="smt_convert --img --imagesize 512x1024 --tilesize 128x128 --tilemap ../data/data.smf ../data/data.smt"

# == Pre Test Actions ==
#nothing

# == Test Action ==
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND

# == Post test actions ==
#nothing
exit $?
