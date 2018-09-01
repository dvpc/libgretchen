#!/bin/bash

if [ ! $1 ]; then
    echo "Missing Arguments:"
    echo "Usage:    $0 input-file"
    exit 1;
fi

build/gret -o sandbash/opt5 &

sleep 1

build/gret -o sandbash/opt5 -f "$1" 

sleep 1
jobs
kill %%

wait
echo "done"


