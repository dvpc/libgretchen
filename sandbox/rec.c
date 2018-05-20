#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "gretchen.backend.h"
#include "gretchen.internal.h"



int main(int argc, char** argv) {

    bool use_stdio = true;
    char c;
    while (1) {
        c = getopt (argc, argv, "f:");
        if (c == -1)
            break;
        switch (c) {
            case 'f':
                use_stdio = false;
                break;
            case '?':
        default:
            printf ("Usage: %s [-f <file to write into>].\n", argv[0]);
        }
    }

    FILE *fhandle = NULL;
    if (use_stdio)
        fhandle = stdout;
    else {
        fhandle = fopen(argv[2], "wb");
        if (!fhandle)
            return 1;
    }



    grt_sigcatch_Init();
   
    size_t buflen = 1 << 15;
    grt_record *rec = grt_record_Create(buflen);
    if (rec == NULL) {
        fprintf(stderr, "cannot init backend!\n");
        return 1;
    }
    Pa_Sleep(150);
    grt_record_StartStream(rec);

    float* popbuf = malloc(8192 * sizeof(float));
    
    while(!grt_sigcatch_ShouldTerminate()) {

        if (grt_record_StreamStatus(rec)!=grtbckNoError) {
            grt_sigcatch_Set(1);
            break;
        }

        size_t len = grt_record_PollNonBlocking(
                        rec, 
                        popbuf, 
                        8192);
        if (len > 0) {
            fwrite(popbuf, sizeof(float), len, fhandle);
        }

        Pa_Sleep(150);
    }
    if (grt_sigcatch_ShouldTerminate()) {
    }

    fclose(fhandle);
    free(popbuf);
    grt_record_StopStreaming(rec);
    grt_record_Destroy(rec);

    return 0;
}





