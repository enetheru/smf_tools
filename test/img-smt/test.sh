#!/bin/bash
COMMAND="smt_convert -vf --smt ../data/image_1.png"

# == Pre Test Actions ==
#nothing

# == Test Action ==
echo "[TEST] Command =  $COMMAND"
RETVAL=( $(eval $TIMER $VALGRIND $COMMAND) )
echo "[TEST] Result = $RETVAL"

# == Post test actions ==
smt_convert -vf --img --tilemap output.smt.csv output.smt

echo $RETVAL
exit $RETVAL

