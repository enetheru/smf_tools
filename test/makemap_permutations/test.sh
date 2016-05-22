#!/bin/bash
OPTIONS=( \
    'p' \
    'o testoutput' \
    'Z' \
    'L' \
    'N "Test Map"' \
    'D "This is a escription"' \
    'y 128' \
    'Y 128' \
    'm ../data/image_1.png' \
    'd ../data/image_1.png' \
    'a ../data/height.tif' \
    'g ../data/height.tif' \
 \ #    'F beans' \
    't ../data/height.tif' \
    'r ../data/height.tif' \
 \ #    'z 0'
)
#COMMAND="smt_convert -v --smt --tilesize 32x32 --imagesize 1024x1024 ../data/image_1.png"

DATA=( 0 "" "" )
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

while [[ $TRUE -ne 255 ]]; 
do

    COMMAND='makemap -vGf -n testmap -w 2 -l 2 -d ../data/image_1.png'
    for ((i=0;i<${#DATA[@]};++i));
    do
        if [[ ${DATA[$i]} -eq 1 ]];
        then
            COMMAND+=" -${OPTIONS[$i]}"
        fi
    done
    echo "[TEST]${DATA[@]}"
    echo -e "[TEST]$COMMAND\n"
    $COMMAND
    RESULT=$?
    echo -e '[TEST] Result=' $RESULT
    if [[ $RESULT -gt 1 ]]
        then
            echo $COMMAND >> failure.txt
            sleep 10
        fi
    echo -e '----------------------------\n'
    RecursiveTest ${#OPTIONS[@]} 0
    TRUE=$?
done

# == Test Action ==
#echo COMMAND = $COMMAND
#eval $TIMER $VALGRIND $COMMAND

# == Post test actions ==
#nothing
exit $?
