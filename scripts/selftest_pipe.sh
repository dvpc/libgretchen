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
OUTPUT_filewav="$OUTPUTDIR/tmp.generated.raw"

# if input is a file handle stuff differently
if [ ! -f $2 ]; then
    INPUT_is_FILE="0"
else
    INPUT_is_FILE="1"
fi


if [ "$INPUT_is_FILE" -eq "0" ]; then  

    OUTPUT="$(echo $2 | build/enc $opt | tee $OUTPUT_filewav | build/dec $opt)"

    if [ $2 == "$OUTPUT" ]; then
        echo "Test OK!"
    else 
        echo "Test Not Ok!"
        echo $2
        echo $OUTPUT 
    fi

else

    cat $2 | build/enc $opt | tee $OUTPUT_filewav | build/dec $opt > $OUTPUT_file

    DIFF=$(diff $2 $OUTPUT_file)
    if [ "$DIFF" != "" ]; then
        echo "Test Not OK!"
    else
        echo "Test OK!"
    fi
    du -h $2
    du -h $OUTPUT_file
    du -h $OUTPUT_filewav
fi



