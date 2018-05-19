#!/bin/bash

bpath="`dirname $BASH_SOURCE`/build"
if [ ! -d "$bpath" ]; then
        mkdir "$bpath"
fi

cd "$bpath"
cmake .. -DCMAKE_BUILD_TYPE=Debug
make

