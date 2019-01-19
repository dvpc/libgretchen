/*
 * Gretchen internal helper test envelope
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

#include "gretchen.internal.h"

int main(int argc, char **argv) {
    if (argc!=2) {
        printf("Usage: %s inputfile\n", argv[0]); 
        return 1;
    }
    char *name = argv[1];
    int64_t filesize;
    int8_t error;
    uint8_t *source = read_binaryfile((uint8_t*)name, &filesize, &error);
    if (error!=0 || source==NULL) {
        printf("File could not be read.\n");  
        return 1;
    }
    printf("\n");
    printf("file read: %s\nsize: %llu\nerror: %d\np: %p \n", 
                    name, 
                    filesize, 
                    error,
                    source);


    envelope_t *env = envelope_create((uint8_t*)name, source);
    /*envelope_t *env = envelope_create(NULL, source);*/
    
    uint16_t hash = hash_djb2(env->source);
    printf("hash: %hu\n", hash);
    
    printf("\npacking envelope...\n");

    uint8_t *envstr;
    envelope_pack(env, &envstr); 
    printf("envelope: %p size: %zu\n", envstr, strlen((char*)envstr));

    envelope_t *uenv;
    envelope_unpack(envstr, &uenv);

    printf("\nunpacking envelope yields....\n");
    envelope_print(uenv);

    uint16_t hash2 = hash_djb2(uenv->source);
    printf("uhash: %hu\n", hash2);

    printf("\nis source and envelope sourece equal?: %d\n", 
                    (strcmp((char*)env->source, (char*)uenv->source)==0));

    int8_t error2;
    envelope_writeout(uenv, (uint8_t*)"test/0", &error2);
    if (error==0)
        printf("File written: %s error:%i\n", uenv->name, error2);
    else
        printf("File NOT written: %s error:%i\n", uenv->name, error2);
    printf("\n");

    envelope_destroy(env);
    envelope_destroy(uenv);

    free(source);
    free(envstr);

    return 0;
}



