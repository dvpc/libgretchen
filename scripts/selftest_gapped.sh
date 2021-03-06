#!/bin/bash

if [ ! $1 ] || [ ! $2 ]; then
    echo "Missing Arguments:"
    echo "Usage:    $0 opt-file input-file"
    exit 1;
fi

opt="$(cat $1)"

OUTPUTDIR="incoming"
if [ ! -d "$OUTPUTDIR" ]; then
    mkdir "$OUTPUTDIR"
fi
OUTPUT_file="$OUTPUTDIR/tmp.output.bin"
OUTPUT_filewav="$OUTPUTDIR/tmp.recorded.raw"
OUTPUT_filewav2="$OUTPUTDIR/tmp.generated.raw"

# TODO output is not written to file...
# did not succeed in digging this out.
build/rec | tee $OUTPUT_filewav | build/dec $opt > $OUTPUT_file &
#build/rec | tee $OUTPUT_filewav | build/dec $opt &
# TODO keep process number to kill in a propper manner later (kill $!)

# encoding and playing the file
sleep 1
cat $2 | build/enc $opt | tee $OUTPUT_filewav2 | build/play

# stop the recording process (moved to the background)
#jobs
sleep 1
kill $! #kill %%

wait
# now compare the original and the generated file
DIFF=$(diff $2 $OUTPUT_file)
if [ "$DIFF" != "" ]; then
    echo "Test Not OK!"
else
    echo "Test OK!"
fi
du -h $2
du -h $OUTPUT_file
du -h $OUTPUT_filewav



