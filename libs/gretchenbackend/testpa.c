
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "portaudio.h"


static void supported_samplerates(const PaStreamParameters *inp,
                                  const PaStreamParameters *out)
{
    static double stdsamplerates[] = {8000.0, 9600.0, 11025.0, 12000.0, 
            16000.0, 22050.0, 24000.0, 32000.0, 44100.0, 48000.0, 88200.0, 
            96000.0, 192000.0, -1 };
    PaError err; 
    for (int32_t i=0; stdsamplerates[i]>0; i++) {
        err = Pa_IsFormatSupported(inp, out, stdsamplerates[i]);
        if (err==paFormatIsSupported)
            printf("%8.2f \n", stdsamplerates[i]);
    }
}



int main(int argc, char** argv) {

    (void) argc;
    (void) argv;


    PaError err = Pa_Initialize();
    if (err != paNoError)
        goto error;

    printf("\n");
    printf("PortAudio version: 0x%08X\n", Pa_GetVersion());
    printf("Version text: '%s'\n", Pa_GetVersionInfo()->versionText );    
    int numDevices = Pa_GetDeviceCount();
    if(numDevices<0) {
        printf( "ERROR: Pa_GetDeviceCount returned 0x%x\n", numDevices );
        err = numDevices;
        goto error;
    }
    printf("Number of devices = %d\n", numDevices);
    const PaDeviceInfo *deviceInfo;
    for (int32_t i=0; i<numDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i); 
        printf("device #%d --------------------------------------------\n",i);
        printf("Name                        = %s\n", deviceInfo->name );
        printf("Host API                    = %s\n",  Pa_GetHostApiInfo( deviceInfo->hostApi )->name );
        printf("Max inputs = %d", deviceInfo->maxInputChannels  );
        printf(", Max outputs = %d\n", deviceInfo->maxOutputChannels  );
        printf("Default low input latency   = %8.4f\n", deviceInfo->defaultLowInputLatency  );
        printf("Default low output latency  = %8.4f\n", deviceInfo->defaultLowOutputLatency  );
        printf("Default high input latency  = %8.4f\n", deviceInfo->defaultHighInputLatency);
        printf("Default high output latency = %8.4f\n", deviceInfo->defaultHighOutputLatency);
        printf("-------------------------------------------------------\n");

        PaStreamParameters inputParameters;
        inputParameters.sampleFormat = paFloat32;
        inputParameters.channelCount = 1;
        inputParameters.device = i;
        inputParameters.hostApiSpecificStreamInfo = NULL;
        inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = NULL;
        PaStreamParameters outputParameters;
        outputParameters.sampleFormat = paFloat32;
        outputParameters.channelCount = 1;
        outputParameters.device = i;
        outputParameters.hostApiSpecificStreamInfo = NULL;
        outputParameters.suggestedLatency = deviceInfo->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = NULL; 

        printf("Supported Samplerates Input\n");
        supported_samplerates(&inputParameters, NULL);
        printf("Supported Samplerates Output\n");
        supported_samplerates(NULL, &outputParameters);
        /*supported_samplerates(&inputParameters, &outputParameters);*/
    }


error:
    Pa_Terminate();
    fprintf(stderr, "Pa err: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    return err; 
}
