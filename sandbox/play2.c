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
        c = getopt (argc, argv, "f:");
        if (c == -1)
            break;
        switch (c) {
            case 'f':
                use_stdio = false;
                break;
            case '?':
        default:
            printf ("Usage: %s [-f <file to play>].\n", argv[0]);
        }
    }
    FILE *fhandle = NULL;
    if (use_stdio)
        fhandle = stdin;
    else {
        fhandle = fopen(argv[2], "rb");
        if (!fhandle) {
            printf("No File!!!\n");
            return 1;
        }
    }

    size_t internbuflen = 1 << 14;
    grtBackend_t* back = grtBackend_create(internbuflen, true);
    if (back == NULL) {
        fprintf(stderr, "cannot init backend.\n");
        return 1;
    }
    // NOTE this Pa_Sleep is needed if more than one backend 
    // (e.g. playback and record at the same time) is used.
    // FIXME investigate is it still needed??
    Pa_Sleep(150);
    int error;
    grtBackend_startstream(back, &error);
    if (error != 0) {
        fprintf(stderr, "backend: cannot start stream.\n");
        grtBackend_destroy(back);
        fclose(fhandle);
        return 1;
    }
    
    size_t buflen = 8192;
    float* buffer = malloc(buflen * sizeof(float));
    size_t avail, nread, len, pushed;
    bool done = false;
    while(!done) {
        avail = grtBackend_push_available(back);
        len = avail < buflen ? avail : buflen;
        nread = fread(buffer, sizeof(float), len, fhandle);  
        pushed = grtBackend_push(back, buffer, nread);
        /*fprintf(stderr, "avail %zu nread %zu pushed %zu \n", avail, nread, pushed);*/
        if (nread!=pushed)
            ; // FIXME error loosing samples!!!
        if (pushed==0)
            done = true;

        Pa_Sleep(150);
    }
    free(buffer);

    grtBackend_stopstream(back, &error);
    grtBackend_destroy(back);
    fclose(fhandle);
    return 0;
}
