#!/bin/bash

if [ ! $1 ] || [ ! $2 ] || [[ ! $3 ]] ; then
    echo "Missing Arguments:"
    echo "Usage:    $0 opt-file noiseopt-file input-file"
    exit 1;
fi

opt="$(cat $1)"
noiseopt="$(cat $2)"

OUTPUT_file="tmp.output.bin"
OUTPUT_filewav="tmp.output.raw"
OUTPUT_filewav2="tmp.output2.raw"

cat $3 | build/enc $opt | tee $OUTPUT_filewav | build/noise $noiseopt | tee $OUTPUT_filewav2 | build/dec $opt > $OUTPUT_file

DIFF=$(diff $3 $OUTPUT_file)
if [ "$DIFF" != "" ]; then
    echo "Test Not OK!"
else
    echo "Test OK!"
fi
du -h $3
du -h $OUTPUT_file
du -h $OUTPUT_filewav



