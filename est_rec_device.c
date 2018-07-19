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



static int _record_callback();

typedef struct {
    unsigned int num_channels;
    unsigned int itcount;
    bool chlimit_reached;
} testdata_t;

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;



    PaError err = Pa_Initialize();
    if (err != paNoError)
        goto error_pa;
    PaStreamParameters strParams;
    strParams.device = Pa_GetDefaultInputDevice();
    if (strParams.device==paNoDevice)
        goto error_pa; 
    // NOTE paNonInterleaved 
    // good info https://stackoverflow.com/questions/35640453/interpreting-inputbuffers-value-in-a-callback    
    strParams.sampleFormat = paFloat32 | paNonInterleaved;
    strParams.suggestedLatency = Pa_GetDeviceInfo(strParams.device)->defaultLowInputLatency;
    strParams.hostApiSpecificStreamInfo = NULL;

    testdata_t* data = malloc(sizeof(testdata_t));
    data->num_channels = 1;
    data->chlimit_reached = false;


    while(true) {

        fprintf(stderr, "Testing with %u channels \n", data->num_channels);

        strParams.channelCount = data->num_channels;
        data->itcount = 0;

        PaStream* stream; 
        err = Pa_OpenStream(
                            &stream, &strParams, NULL,
                            44100, paFramesPerBufferUnspecified,
                            paDitherOff | paClipOff, 
                            _record_callback,
                            data); 
        if (err!=paNoError)
            goto error_openstream;

        err = Pa_StartStream(stream);
        if (err!=paNoError)
            goto error_startstream;

        Pa_Sleep(10);

        Pa_StopStream(stream);
        Pa_CloseStream(stream);

        fprintf(stderr, "Test %s\n", (data->chlimit_reached?"Not Ok":"Ok"));


        if (data->chlimit_reached)
            goto error_errordata;

        data->num_channels ++;
    }


    goto return_ok;


    error_pa:
        Pa_Terminate();
        fprintf(stderr, "Portaudio error %s\n", Pa_GetErrorText(err));
        goto return_err;
    error_openstream:
        Pa_Terminate();
        fprintf(stderr, "Could not open stream with %u channels %s\n", data->num_channels, Pa_GetErrorText(err));
        goto return_err;
    error_startstream:
        Pa_Terminate();
        fprintf(stderr, "Could start stream with %u channels %s\n", data->num_channels, Pa_GetErrorText(err));
        goto return_err;
    error_errordata:
        Pa_Terminate();
        fprintf(stderr, "The device seems to have %u input channels %s\n", data->num_channels-1, Pa_GetErrorText(err));
        goto return_ok;

    return_err:
        return 1;

    return_ok:
        return 0;
}




static int _record_callback(
                const void *inbuf, 
                const void *outbuf,
                unsigned long frmsPerBuf, 
                const PaStreamCallbackTimeInfo* timeInfo,
                PaStreamCallbackFlags statFlags,
                void *user)
{
    (void) outbuf;
    (void) statFlags;
    (void) timeInfo;
    (void) user;

    testdata_t* data = (testdata_t*) user;


    if (inbuf == NULL)
        fprintf(stderr, "callback input is NULL\n");
    else {
        size_t chlen = frmsPerBuf / data->num_channels;
        float* interlv = (float*) inbuf;
        for (unsigned int j=0; j<data->num_channels; j++) {
            fprintf(stderr,"c%u: ", j); 
            bool all_zero = true;
            float* chp = ((float**) interlv)[j];
            for (size_t k=0; k<chlen; k++) {
                all_zero = chp[k]==0?true:false;
                fprintf(stderr, "%f(%s)", chp[k], (chp[k]==0?"y":"n"));
                if (!all_zero)
                    break;
            } 
            fprintf(stderr,"\n"); 
            if (all_zero) {
                data->chlimit_reached = true;
            }
        }
    }
    return paComplete;
    /*data->itcount ++; */
    /*if (data->itcount > 3)*/
        /*return paComplete;*/
    /*else*/
        /*return paContinue;*/
}
