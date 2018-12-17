#!/bin/bash

if [ ! $1 ]; then
    echo "Missing Arguments:"
    echo "Usage:    $0 opt-file input-file"
    exit 1;
fi

build/gret -o sandbash/opt4 -o "$1" &

sleep 1

build/gret -o sandbash/opt4 -o "$1" -f "$2" 

sleep 1
jobs
kill %%

wait
echo "done"


