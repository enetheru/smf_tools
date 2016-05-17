#!/bin/bash
shopt -s extglob

for i in !(data)
do
    if [[ -d $i ]]
    then
        cd $i
        BEAN=( !(test.sh) )
        if [[ ${#BEAN[@]} > 1 ]]
        then
            rm !(test.sh)
        fi
        cd ../
    fi
done

shopt -u extglob
