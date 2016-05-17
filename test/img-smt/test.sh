#!/bin/bash
COMMAND="smt_convert -vf --smt ../data/image_1.png"

# == Pre Test Actions ==
#nothing

# == Test Action ==
echo COMMAND = $COMMAND
RETVAL=`eval $TIMER $VALGRIND $COMMAND`

# == Post test actions ==
#smt_convert -f --img output.smt


exit $RETVAL

