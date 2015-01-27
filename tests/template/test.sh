#!/bin/bash
COMMAND="smt_convert -v --smt --tilesize 32x32 --imagesize 1024x1024 ../data/image_1.png"

# == Pre Test Actions ==
#nothing

# == Test Action ==
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND

# == Post test actions ==
#nothing
exit $?