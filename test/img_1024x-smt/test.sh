#!/bin/bash
COMMAND="smt_convert -vf --smt ./test.jpg"

# == Pre Test Actions ==
oiiotool --resample 1024x1024 ../data/image_1.png -o test.jpg

# == Test Actions ==
echo COMMAND = $COMMAND
eval $TIMER $VALGRIND $COMMAND

# == Post Test Actions ==
# Nothing

# exit
exit $?
