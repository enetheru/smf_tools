#!/bin/bash

OPTIONS=( \
    'o testoutput' \
    'c' \
    'y 128' \
    'Y 128' \
    'm ../data/image_1.png' \
    'D ../data/image_1.png' \
    'H ../data/height.tif' \
    'G ../data/height.tif' \
 \ #    'F beans' \
    'T ../data/height.tif' \
    'R ../data/height.tif' \
 \ #    'w 0'
)

DATA=( 0 0 )
RecursiveTest()
{
    if [[ $2 -ge $1 ]]; then return 255; fi
    if [[ ${DATA[$2]} != 1 ]]
    then
        DATA[$2]=1
        return 1
    fi
    RecursiveTest $1 $(($2+1))
    if [[ $? -eq 255 ]]
    then return 255
    elif [[ $? -eq 1 ]]
    then DATA[$2]=0
    fi
    return 0
}

# == Test  ==
while [[ $TRUE -ne 255 ]]; 
do
    NAME='map'
    for ((j=0;j<${#DATA[@]};++j));
    do
        NAME+=${DATA[$j]}
    done
    COMMAND="makemap -vgO -n "${NAME}" -x 4 -z 4 -D ../data/image_1.png"
    for ((i=0;i<${#DATA[@]};++i));
    do
        if [[ ${DATA[$i]} -eq 1 ]];
        then
            COMMAND+=" -${OPTIONS[$i]}"
        fi
        if [[ $i -eq 0 && ${DATA[$1]} -eq 1 ]]
        then
            OUTPUT_PATH='testoutput/'
        else
            OUTPUT_PATH=''
        fi
    done
    echo "testing for: ${OUTPUT_PATH}${NAME}.sdd"
    if [[ -e "${OUTPUT_PATH}${NAME}.sdd" ]];
    then
       echo "[TEST]skipping $NAME"
    else  
        echo "[TEST]${DATA[@]}"
        echo -e "[TEST]$COMMAND\n"
        eval "$COMMAND"
        RESULT=$?
        echo -e '[TEST] Result=' $RESULT
        if [[ $RESULT -gt 1 ]]
            then
                echo $COMMAND >> failure.txt
                read -r -s -t 10 -p "press enter to continue" CONTINUE
            fi
        echo -e '----------------------------\n'
    fi
    RecursiveTest ${#OPTIONS[@]} 0
    TRUE=$?
done

if [[ -e failure.txt ]]; then exit 1;fi
exit $?
