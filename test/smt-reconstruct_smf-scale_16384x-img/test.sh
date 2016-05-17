#!/bin/bash
SCALEWIDTH=16384
SCALEHEIGHT=$SCALEWIDTH

COMMAND="smt_convert -vf --img --imagesize $SCALEWIDTHx$SCALEHEIGHT --tilemap ../data/data.smf ../data/data.smt"

# == Pre Test Actions ==
#nothing

# == Test Action ==
echo "[TEST] = $COMMAND"
RESULT=$(eval $TIMER $VALGRIND $COMMAND)

# == Post test actions ==
OUTSIZE=( $(oiiotool --info "tile_000000.tif" \
    | awk 'BEGIN{ FS="[ :x,]+" }; { print $2 " " $3 }') )
if [ "$SCALEWIDTH" -ne "${OUTSIZE[0]}" ] \
    || [ "$SCALEHEIGHT" -ne "${OUTSIZE[1]}" ]
then
    echo "[TEST] tile_000000.tif is ${OUTSIZE[0]}x${OUTSIZE[1]}, wanted ${SCALEWIDTH}x${SCALEHEIGHT}"
    RESULT=1
fi

#nothing
exit $RESULT

