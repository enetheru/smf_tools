#!/bin/bash
shopt -s extglob

for i in !(data)
do
    if [[ -d $i ]]
    then
        cd $i
        rm !(test.sh)
        cd ../
    fi
done
