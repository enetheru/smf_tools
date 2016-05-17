#!/bin/bash
COMMAND="smt_convert -vf --img ../data/data.smt"

# Pre Test Action
# ===============

# Test Action
# ===========
echo COMMAND = $COMMAND
RETVAL=`eval $TIMER $VALGRIND $COMMAND`

# Post Test Action
# ================


exit $RETVAL
