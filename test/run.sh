#!/bin/bash
export TIMER="`which time` -o time -f '%E'"

if [ "$1" = "--help" ]
then
    echo \
"Testing harness for smf_tools
  Options:
    --help          displays this help
    --leak-check    run commands using valgrind"
    exit 0
fi

if [ "$1" = "--leak-check" ]
then
    export VALGRIND="valgrind --leak-check=full --suppressions=$(find `pwd` -name suppressions.supp)"
fi

#FIXME, if no arguments supplied then run all tests

shopt -s extglob
for i in !(data|template|$1)
do
    if [[ -d $i && $@ =~ $i ]]
    then
        echo -n "TEST: $i "
        cd $i
        ./test.sh &> log.txt
        if [ $? -eq 0 ]; then
            echo -e "\033[70G  \033[32m $(cat time)\033[0m"
        else
            echo -e "\033[70G      \033[31mFAIL\033[0m"
            #echo -e "\nLog of failed entry:"
            #cat log.txt
            #echo -e "\n"
        fi
        cd ../
    fi
done

echo -e "Completed :)"

shopt -u extglob
