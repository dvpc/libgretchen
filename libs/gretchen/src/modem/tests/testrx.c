/*
 * Gretchen test rx
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

void progress_callback(
                uint16_t hash,
                uint16_t frame_num,
                uint16_t frame_nummax,
                int payload_valid, 
                void* user) {
    (void) user;
    printf("rx progress callback: hash %hu num %i max %i payloadvalid %i\n", 
                    hash, frame_num, frame_nummax, payload_valid);
}


void filecomplete_callback(
                uint8_t* filename, 
                uint8_t* source, 
                size_t sourcelen, 
                void* user) {
    (void) source;
    (void) user;
    printf("rx file complete: name %s len %zu \n", 
                    filename, sourcelen);

    char *path = "test/\0";
    char *name = malloc(sizeof(uint8_t)*(strlen(path)+strlen((char*)filename))+2);
    strcpy(name, path);
    // FIXME this filesystemdelimiterstuff is hardly platform independent
    // solve or factor out
    /*strcat(name, "_");*/
    // i could require that path ends with '/' or (see above) legel delim
    strcat(name, (char*)filename);
    int8_t error;
    write_binaryfile((uint8_t*)name, source, &error);
    printf("File written with error %i \n", error);
    free(name);
}




int main(int argc, char **argv) {
    if (argc!=2) {
        printf("Usage: %s inputfile\n", argv[0]); 
        return 1;
    }

    grtModemOpt_t* opt = grtModemOpt_create_default(44100);

    // TODO buffersize
    gretchenRX_t* rx = gretchenRX_create(opt, 1 << 14);
    rx->callback = filecomplete_callback;
    rx->prog_callback = progress_callback;

    // TODO buffersize
    size_t chunk_want = 1 << 12;
    float* chunkbuf = malloc(sizeof(float)*chunk_want);

    // simulating incoming chunks from an audio device
    FILE *fp = fopen(argv[1], "rb");
    if (fp==NULL)
        goto cleanup;

    int8_t error;
    size_t k=0;
    while(true) {    
        size_t rlen = fread(chunkbuf, sizeof(float), chunk_want, fp);  
        k+=rlen;
        fseek(fp, 0L, k);
        gretchenRX_push_le16f(rx, chunkbuf, rlen, &error);
        if (rlen==0)
            break;
    }

    cleanup:
        free(chunkbuf);
        if (fp)
            fclose(fp);
        else
            printf("Error\n");
        gretchenRX_destroy(rx);
        grtModemOpt_destroy(opt);
    return 0;
}



