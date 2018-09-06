#include <stdio.h>
#include <stdlib.h>

#include "gretchen.internal.h"

int main(int argc, char **argv) {
    if (argc!=2) {
        printf("Usage: %s inputfile\n", argv[0]); 
        return 1;
    }
    uint8_t *name = argv[1];
    long filesize;
    int error;
    uint8_t *source = read_binary_file(name, &filesize, &error);
    if (error!=0 || source==NULL) {
        printf("File could not be read.\n");  
        return 1;
    }
    printf("\n");
    printf("file read: %s\nsize: %zu\nerror: %d\np: %p \n", 
                    name, 
                    filesize, 
                    error,
                    source);


    envelope_t *env = envelope_create(name, source);
    
    uint64_t hash = hash_djb2(env->source);
    printf("hash: %zu\n", hash);
    
    printf("\npacking envelope...\n");

    uint8_t *envstr;
    envelope_pack(env, &envstr); 
    printf("envelope: %p size: %zu\n", envstr, strlen(envstr));

    envelope_t *uenv;
    envelope_unpack(envstr, &uenv);

    printf("\nunpacking envelope yields....\n");
    envelope_print(uenv);

    uint64_t hash2 = hash_djb2(uenv->source);
    printf("uhash: %zu\n", hash2);

    printf("\nis source and envelope sourece equal?: %d\n", 
                    (strcmp(env->source, uenv->source)==0));

    int error2;
    envelope_writeout(uenv, "test/", &error2);
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



