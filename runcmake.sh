#!/bin/bash

if [[ "$OSTYPE" != darwin* && "$BASH_SOURCE" == "" ]]; then
BASH_SOURCE="$0"
fi

bpath="`dirname $BASH_SOURCE`/build"
if [ ! -d "$bpath" ]; then
        mkdir "$bpath"
fi

cd "$bpath"
cmake .. 
make

