#!/bin/bash
export TIMER="`which time` -o time -f '%E'"
if [ "$1" = "--leak-check" ]
then
    export VALGRIND="valgrind --leak-check=full --suppressions=$(find `pwd` -name suppressions.supp)"
fi

shopt -s extglob
for i in !(data|template|$1)
do
    if [[ -d $i ]]
    then
        echo -n "TEST: $i "
        cd $i
        ./test.sh &> log.txt
        if [ $? -eq 0 ]; then
            echo -e "\033[40G  $(cat time)"
        else
            echo -e "\033[40G    FAIL"
        fi
        cd ../
    fi
done
