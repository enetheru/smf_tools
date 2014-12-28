#!/bin/bash
for i in *
do
    if [[ -d $i ]]
    then
        echo -n "TEST: $i "
        cd $i
        ./test.sh &> log.txt
        if [ $? -eq 0 ]; then
            echo OK
        else
            echo FAIL
        fi
        cd ../
    fi
done
