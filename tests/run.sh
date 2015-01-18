#!/bin/bash
shopt -s extglob
for i in !(data)
do
    if [[ -d $i ]]
    then
        echo -n "TEST: $i "
        cd $i
        ./test.sh &> log.txt
        if [ $? -eq 0 ]; then
            echo -e "\033[40GOK"
        else
            echo -e "\033[40GFAIL"
        fi
        cd ../
    fi
done
