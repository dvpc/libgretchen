#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "grt_backend.h"


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



    size_t buflen = 1 << 14;
    grt_playback *play = grt_playback_Create(buflen);
    if (play == NULL) {
        fprintf(stderr, "cannot init backend!\n");
        return 1;
    }
    Pa_Sleep(150);
    grt_playback_StartStream(play);  

    size_t pushlen = 8192;
    float* pushbuf = malloc(pushlen * sizeof(float));

    bool done = false;
    while(!done) {
        size_t avail = grt_playback_PushAvailable(play);
        size_t safelen = avail < pushlen ? avail : pushlen;

        size_t nread = fread(
                         pushbuf, 
                         sizeof(float), 
                         safelen, 
                         fhandle);

        size_t pushed = grt_playback_PushNonBlocking(
                            play, 
                            pushbuf, 
                            nread); 
        if (pushed==0 && nread==0)
            done = true;
        // TODO
        // investigate i seems so unstable...
        // and if i am loosing data here hmmm
        Pa_Sleep(150);
    }

    fclose(fhandle);
    free(pushbuf);
    grt_playback_StopStreaming(play);
    grt_playback_Destroy(play);

    fclose(fhandle);

    return 0;
}


