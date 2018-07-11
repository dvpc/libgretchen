#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "gretchen.backend.h"


int main(int argc, char** argv) {
    bool use_stdio = true;
    bool rec_stereo = false;
    char c;
    while (1) {
        c = getopt (argc, argv, "f:s:");
        if (c == -1)
            break;
        switch (c) {
            case 'f':
                use_stdio = atoi(optarg);
                break;
            case 's':
                rec_stereo = atoi(optarg);
                break;
            case '?':
        default:
            printf ("Usage: %s [-f <file to write into> -s <record {0 mono, 1 stereo}].\n", argv[0]);
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

    size_t internbuflen = 1 << 14;
    grtBackend_t* back = grtBackend_create(internbuflen, false, rec_stereo);
    if (back == NULL) {
        fprintf(stderr, "cannot init backend (rec).\n");
        return 1;
    }
    // NOTE this Pa_Sleep is needed if more than one backend 
    // (e.g. playback and record at the same time) is used.
    // FIXME investigate is it still needed??
    Pa_Sleep(150);
    int error;
    grtBackend_startstream(back, &error);
    if (error != 0) {
        fprintf(stderr, "backend (rec): cannot start stream. \n");
        grtBackend_destroy(back);
        fclose(fhandle);
        return 1;
    }

    grtSigcatcher_Init();
    size_t asklen = 8192 * (rec_stereo?2:1);
    float* buffer = NULL;
    size_t nread;
    while(!grtSigcatcher_ShouldTerminate()) {
        if (!grtBackend_isstreamactive(back)) {
            grtSigcatcher_Set(1);
            break;
        }
        grtBackend_poll(back, asklen, &buffer, &nread);
        fprintf(stderr, "ask %zu nread %zu buff %p \n", asklen, nread, buffer);
        if (buffer!=NULL && nread>0)
            fwrite(buffer, sizeof(float), nread, fhandle);

        Pa_Sleep(200); 
    }

    grtSigcatcher_Destroy();
    grtBackend_stopstream(back, &error);
    grtBackend_destroy(back);
    fclose(fhandle);
    return 0;
}






