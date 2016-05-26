#!/bin/bash

declare -A OPTIONS
#OFF"  -q  \t--quiet\t"
#ON"  -v  \t--verbose\t"
#ON"  -p  \t--progress\t"
# "  -o  \t--output <dir>\t"
OPTIONS[0,1]="./relative/"
OPTIONS[0,2]="`pwd`/absolute/"
OPTIONS[0,3]="`pwd`/with spaces/"
#"  -n  \t--name <filename>\t"
OPTIONS[1,1]="nospaces"
OPTIONS[1,2]="with spaces"
#ON"  -O  \t--overwrite\t"
#"  -i  \t--imagesize=XxY\t"
OPTIONS[2,1]="-i 1024x1024" # square
OPTIONS[2,2]="-i 512x1024" # portrait
OPTIONS[2,3]="-i 1024x512" # landscape
#"  -t  \t--tilesize=XxY\t"
OPTIONS[3,1]="-t 32x32" # default
OPTIONS[3,2]="-t 128x128" # large
#"  -f  \t--format=[DXT1,RGBA8,USHORT]\t"
OPTIONS[4,1]="-f DXT1"
OPTIONS[4,2]="-f RGBA8"
OPTIONS[4,3]="-f USHORT"
#"  -M  \t--tilemap=<csv|smf>\t"
OPTIONS[5,1]="-M ../data/tilemap.csv"
OPTIONS[5,2]="-M ../data/data.smf"
#"  -e  \t--filter=1,2-n\t"
OPTIONS[6,1]=""
#"  -k  \t--overlap=0\t"
OPTIONS[7,1]=""
#"  -b  \t--border=0\t"
OPTIONS[8,1]=""
#"  -d  \t--dupli=[None,Exact,Perceptual]\t"
OPTIONS[9,1]=""
#      "\t--smt\t"              "Save tiles to smt file"
#      "\t--img\t"              "Save tiles as images"
OPTIONS[10,1]="--smt"
OPTIONS[10,2]="--img"

OPTIONS[11,1]="../data/image_1.png"
OPTIONS[11,2]="../data/data.smt"
OPTIONS[11,3]="../data/tiles/*"


#             0 1 2 3 4 5 6 7 8 9 10 11
OPTIONS_NUM=( 3 2 3 2 3 2 0 0 0 0  2  3 )

DATA=( 0 )
RecursiveTest() #($1=width, $2=position)
{
    if (( $2 > $1 )); then return 255; fi #we've finished counting
    if (( DATA[$2] < OPTIONS_NUM[$2] ))
    then
        (( DATA[$2] = DATA[$2] + 1 ))
        return
    fi

    RecursiveTest $1 $(($2+1))

    if [[ $? == 255 ]]; then return 255 #we've finished counting
    else
        DATA[$2]=0
    fi
}

# == Test  ==

mkdir relative absolute 'with spaces'
while [[ $TRUE -ne 255 ]];
do
    echo ""
    #compulsory options
    if (( DATA[10] == 0 )); then DATA[10]=1; fi
    if (( DATA[11] == 0 )); then DATA[11]=1; fi

    # setup
    TEST_NO=
    COMMAND="smt_convert -vpO "

    # build TEST_NO
    for (( i = 0; i < ${#OPTIONS_NUM[@]}; ++i ));
    do TEST_NO+=${DATA[$i]}; done
    echo "[TEST] TEST_NO=$TEST_NO"

    # build -o switch
    OUTPUT_PATH=
    if (( DATA[0] != 0 )); then
        OUTPUT_PATH="${OPTIONS[0,${DATA[0]}]}"
    else
        OUTPUT_PATH="./"
    fi

    # build -n switch
    OUTPUT_NAME=
    if (( DATA[1] != 0 )); then
        OUTPUT_NAME="${TEST_NO}_${OPTIONS[1,${DATA[1]}]}"
    else
        OUTPUT_NAME="output.smt"
    fi

    # build COMMAND
    for (( i = 0; i < ${#OPTIONS_NUM[@]}; ++i ));
    do
        if (( DATA[$i] == 0 )); then continue;
        elif (( $i == 0 )); then
            COMMAND+=" -o $OUTPUT_PATH"
        elif (( $i == 1 )); then
            COMMAND+=" -n $OUTPUT_NAME"
        else
            COMMAND+=" ${OPTIONS[$i,${DATA[$i]}]}"
        fi
    done
    echo "[TEST] COMMAND=$COMMAND"

    # skip conditions
    if [[ 0 -eq 1 ]]; #condition to skip test
    then
        echo "[TEST]skipping $TEST_NO"
    fi

    #run COMMAND
    eval "$COMMAND"
    COMMAND_RESULT=$?

    # Check COMMAND_RESULT
    if (( COMMAND_RESULT > 1 )) #COMMAND FAILURE
    then
        echo $COMMAND >> failure.txt
        echo "[CHECK] (( COMMAND_RESULT > 1 )) = Fail"
        read -r -s -t 10 -p "press enter to continue" CONTINUE
        FAIL=1
    elif (( COMMAND_RESULT == 1 )) #COMMAND FAILURE due to input parameters
    then
        echo "[CHECK] (( COMMAND_RESULT > 1 )) = Fail"
        FAIL=1
    else # COMMAND SUCCESS
        # - files exist in the proper places
        if [[ -e ${OUTPUT_PATH}${OUTPUT_NAME} ]]
        then
            echo "[CHECK] [[ -e \${OUTPUT_PATH}\${OUTPUT_NAME} ]] =  Success"
        else
            echo $COMMAND >> failure.txt
            echo "[CHECK] [[ -e \${OUTPUT_PATH}\${OUTPUT_NAME} ]] =  Fail"
            read -r -s -t 10 -p "press enter to continue" CONTINUE
            FAIL=1
        fi
    fi

    # cleanup
    if (( FAIL == 0 ))
    then
        rm ${OUTPUT_PATH}${OUTPUT_NAME}
    fi

    RecursiveTest ${#OPTIONS_NUM[@]} 0
    TRUE=$?
done

if [[ -e failure.txt ]]; then exit 1;fi
exit $?
