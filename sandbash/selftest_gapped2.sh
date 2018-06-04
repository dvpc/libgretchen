#!/bin/bash

if [ ! $1 ]; then
    echo "Missing Arguments:"
    echo "Usage:    $0 input-file"
    exit 1;
fi

build/gret &

sleep 1

build/gret -f "$1"

jobs
sleep 1
kill %%

wait
echo "done"


