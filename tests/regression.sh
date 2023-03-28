#!/bin/bash
# regression test variant bench numbers
# arguments: ./old_engine ./new_engine variant1 variant2 variant3 ...

echo "variant $1 $2"
for var in "${@:3}"
do
    ref=`$1 bench $var 2>&1 | grep "Nodes searched  : " | awk '{print $4}'`
    signature=`$2 bench $var 2>&1 | grep "Nodes searched  : " | awk '{print $4}'`
    if [ -z "$ref" ]; then
        echo "${var} none ${signature} <-- no reference"
    elif [ -z "$signature" ]; then
        echo "${var} ${ref} none <-- no new"
    elif [ "$ref" != "$signature" ]; then
        echo "${var} ${ref} ${signature} <-- mismatch"
    else
        echo "${var} ${ref} OK"
    fi
done
