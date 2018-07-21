/* Estimating the number of recording channels

It seems that it is not possible in portaudio to 
query the exact amount of input channels.  
https://stackoverflow.com/questions/40176632/how-to-discover-a-list-of-physical-audio-input-channels

start with n = 1

while no error

set stream parameters with n
open stream 
start stream
    ?? how long should it run?
in recording callback check data
    ?? how

cleanup

return the number of successful channels...

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "gretchen.backend.h"



int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    PaError err = Pa_Initialize();
    if (err != paNoError)
        return 1;

    fprintf(stderr, "Estimating the actual number of input channels...\n");

    int error;
    int num_channel;
    grtBackend_estimate_inputdecive_numchannels(
                    Pa_GetDefaultInputDevice(), 
                    &num_channel, 
                    &error);
    

    Pa_Terminate();

    if (error!=paNoError) {
        fprintf(stderr, "Portaudio error %s\n", Pa_GetErrorText(err));
        return 1;
    } else {
        fprintf(stderr, "The device seems to have %u input channels %s\n", 
                        num_channel, Pa_GetErrorText(err));
        return 0;
    }

}



