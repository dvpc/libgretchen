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



typedef struct {
    unsigned int num_channels;
    bool chlimit_reached;
} estdata_t;

static int _est_record_callback();

void est_num_input_channels(PaDeviceIndex device, int* result_num_channel, int* error)
{
    *error = 0;
    *result_num_channel = 0;
    PaStreamParameters strParams;
    strParams.device = device;
    if (strParams.device==paNoDevice)
        goto error_pa; 
    // NOTE paNonInterleaved 
    // good info https://stackoverflow.com/questions/35640453/interpreting-inputbuffers-value-in-a-callback    
    strParams.sampleFormat = paFloat32 | paNonInterleaved;
    strParams.suggestedLatency = Pa_GetDeviceInfo(strParams.device)->defaultLowInputLatency;
    strParams.hostApiSpecificStreamInfo = NULL;

    
    estdata_t* data = malloc(sizeof(estdata_t));
    data->num_channels = 1;
    data->chlimit_reached = false;

    PaError err = paNoError;

    while(true) {

        fprintf(stderr, "Testing with %u channels \n", 
                            data->num_channels);

        strParams.channelCount = data->num_channels;

        PaStream* stream; 
        err = Pa_OpenStream(&stream, &strParams, NULL,
                            44100, paFramesPerBufferUnspecified,
                            paDitherOff | paClipOff, 
                            _est_record_callback,
                            data); 
        if (err!=paNoError) {
            *error = err;
            break;
        }

        err = Pa_StartStream(stream);
        if (err!=paNoError) {
            *error = err;
            break;
        }

        Pa_Sleep(10);

        Pa_StopStream(stream);
        Pa_CloseStream(stream);

        fprintf(stderr, "Test %s\n", 
                            (data->chlimit_reached?"Not Ok":"Ok"));

        if (data->chlimit_reached) {
            *error = err;
            *result_num_channel = data->num_channels-1;
            break;
        }

        data->num_channels ++;
    }
    free(data);

error_pa:
    ;
}


int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    PaError err = Pa_Initialize();
    if (err != paNoError)
        goto error_pa;

    int error;
    int num_channel;
    est_num_input_channels(Pa_GetDefaultInputDevice(), &num_channel, &error);

    if (error!=paNoError)
        goto error_pa;
    else
        goto result_ok;


    error_pa:
        Pa_Terminate();
        fprintf(stderr, "Portaudio error %s\n", Pa_GetErrorText(err));
        return 1;
    result_ok:
        Pa_Terminate();
        fprintf(stderr, "The device seems to have %u input channels %s\n", num_channel, Pa_GetErrorText(err));
        return 0;
}


static int _est_record_callback(
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

    estdata_t* data = (estdata_t*) user;

    if (inbuf == NULL)
        return paComplete;
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
}


/*static int _record_callback(*/
                /*const void *inbuf, */
                /*const void *outbuf,*/
                /*unsigned long frmsPerBuf, */
                /*const PaStreamCallbackTimeInfo* timeInfo,*/
                /*PaStreamCallbackFlags statFlags,*/
                /*void *user)*/
/*{*/
    /*(void) outbuf;*/
    /*(void) statFlags;*/
    /*(void) timeInfo;*/
    /*(void) user;*/

    /*testdata_t* data = (testdata_t*) user;*/


    /*if (inbuf == NULL)*/
        /*fprintf(stderr, "callback input is NULL\n");*/
    /*else {*/
        /*size_t chlen = frmsPerBuf / data->num_channels;*/
        /*float* interlv = (float*) inbuf;*/
        /*for (unsigned int j=0; j<data->num_channels; j++) {*/
            /*fprintf(stderr,"c%u: ", j); */
            /*bool all_zero = true;*/
            /*float* chp = ((float**) interlv)[j];*/
            /*for (size_t k=0; k<chlen; k++) {*/
                /*all_zero = chp[k]==0?true:false;*/
                /*fprintf(stderr, "%f(%s)", chp[k], (chp[k]==0?"y":"n"));*/
                /*if (!all_zero)*/
                    /*break;*/
            /*} */
            /*fprintf(stderr,"\n"); */
            /*if (all_zero) {*/
                /*data->chlimit_reached = true;*/
            /*}*/
        /*}*/
    /*}*/
    /*return paComplete;*/
/*}*/
