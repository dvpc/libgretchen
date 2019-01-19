/*
 * Gretchen play
 *
 * Copyright (c) 2018 - 2019 Daniel von Poschinger
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
                use_stdio = atoi(optarg);
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
    grtBackend_t* back = grtBackend_create(internbuflen, true, 48000);
    if (back == NULL) {
        fprintf(stderr, "cannot init backend (play).\n");
        return 1;
    }
    // NOTE this Pa_Sleep is needed if more than one backend 
    // (e.g. playback and record at the same time) is used.
    // FIXME investigate is it still needed??
    Pa_Sleep(150);
    int8_t error;
    grtBackend_startstream(back, &error);
    if (error != 0) {
        fprintf(stderr, "backend (play): cannot start stream.\n");
        grtBackend_destroy(back);
        fclose(fhandle);
        return 1;
    }
    
    size_t buflen = 1 << 14;
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
            fprintf(stderr, "backend (play): loosing %zu samples.", nread-pushed);
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
