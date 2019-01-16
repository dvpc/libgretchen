#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "gretchen.backend.h"


int main(int argc, char** argv) {
    bool use_stdio = true;
    char c;
    while (1) {
        c = getopt (argc, argv, "f:s:");
        if (c == -1)
            break;
        switch (c) {
            case 'f':
                use_stdio = atoi(optarg);
                break;
            case '?':
        default:
            printf ("Usage: %s [-f <file to write into> -s\n", argv[0]);
        }
    }
    FILE *fhandle = NULL;
    if (use_stdio)
        fhandle = stdout;
    else {
        fhandle = fopen(argv[2], "wb");
        if (!fhandle) {
            printf("No File!!!\n");
            return 1;
        }
    }

    size_t internbuflen = 1 << 15;
    grtBackend_t* back = grtBackend_create(internbuflen, false, 96000);
    if (back == NULL) {
        fprintf(stderr, "cannot init backend (rec).\n");
        return 1;
    }
    // NOTE this Pa_Sleep is needed if more than one backend 
    // (e.g. playback and record at the same time) is used.
    // FIXME investigate is it still needed??
    Pa_Sleep(150);
    int8_t error;
    grtBackend_startstream(back, &error);
    if (error != 0) {
        fprintf(stderr, "backend (rec): cannot start stream. \n");
        grtBackend_destroy(back);
        fclose(fhandle);
        return 1;
    }
    // FIXME
    // getting number of input channels temporarily
    uint32_t num_channels = back->strParams.channelCount;
    fprintf(stderr, "using %u imput channels...\n", num_channels);

    grtSigcatcher_Init();
    size_t asklen = 1<<16;//8192 * 4;
    float* buffer = NULL;
    float* monobuf = malloc(sizeof(float)*asklen);
    size_t nread;
    while(!grtSigcatcher_ShouldTerminate()) {
        if (!grtBackend_isstreamactive(back)) {
            fprintf(stderr, ".. Recording stream stopped. %s \n", 
                            grtBackend_getstatustext(back));
            grtSigcatcher_Set(1);
            break;
        }
        grtBackend_poll(back, asklen, &buffer, &nread);
        /*fprintf(stderr, "ask %zu nread %zu buff %p \n", asklen, nread, buffer);*/

        // FIXME 
        // explicit polling of the first channel only
        // just for testing purposes
        if (buffer!=NULL && nread>0) {
            size_t idx=0;
            for (size_t k=0; k<nread; k+=num_channels) {
                monobuf[idx] = buffer[k];
                idx++;
            }
            fwrite(monobuf, sizeof(float), idx, fhandle); 
            /*fwrite(buffer, sizeof(float), nread, fhandle);*/
        }
        Pa_Sleep(150); 
    }

    free(monobuf);
    grtSigcatcher_Destroy();
    grtBackend_stopstream(back, &error);
    grtBackend_destroy(back);
    fclose(fhandle);
    return 0;
}






