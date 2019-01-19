/*
 * Gretchen test tx
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

#include "gretchen.h"


#define MAX_SUGGESTED_FILESIZE 1 << 20 // 1048576 bytes ~ 1 MB


int main(int argc, char **argv) {
    if (argc!=2) {
        printf("Usage: %s inputfile\n", argv[0]); 
        return 1;
    }

    grtModemOpt_t* opt = grtModemOpt_create_default(44100);

    // TODO buffersizes... and internal chunk sizes...
    gretchenTX_t* tx = gretchenTX_create(opt, 1<<12);
    int8_t error;

    gretchenTX_inspect_t* info;
    gretchenTX_inspect(tx, (uint8_t*)argv[1], &error, &info);
    if (info==NULL)
        goto label_cleanup;


    int is_toolarge;
    if (info->filesize_bytes > MAX_SUGGESTED_FILESIZE)
        is_toolarge = 1;
    else
        is_toolarge = 0;
    fprintf(stderr, "\n");
    fprintf(stderr, "info file: toolarge %i bytes %zu est-samples: %zu est-time-sec: %zu\n", 
                    is_toolarge, info->filesize_bytes, info->est_encodedsize_samples, info->est_transfer_sec);


    gretchenTX_prepare(tx, (uint8_t*)argv[1], &error);
    fprintf(stderr, "\n");
    fprintf(stderr, "file prepared: %i error \n", error);


    float* samplebuf;
    size_t samplebuflen;
    gretchenTX_get(tx, &samplebuf, &samplebuflen);

    fprintf(stderr, "\n");
    fprintf(stderr, "getting sample buffersize %zu\n", samplebuflen);
    fprintf(stderr, "\n");

    if (samplebuf==NULL)
        goto label_cleanup;
    
    write_rawfile((uint8_t*)"test/output.raw\0", samplebuf, samplebuflen, &error);
    fprintf(stderr, "Output written as test/output.raw with error %i \n", error);

    free(samplebuf); 

    label_cleanup: 
        free(info);
        gretchenTX_destroy(tx);
        grtModemOpt_destroy(opt);

    return 0;
}
