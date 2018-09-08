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
    gretchenTX_inspect(tx, argv[1], &error, &info);
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


    gretchenTX_prepare(tx, argv[1], &error);
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
    
    write_raw_file("test/output.raw\0", samplebuf, samplebuflen, &error);
    fprintf(stderr, "Output written as test/output.raw with error %i \n", error);

    free(samplebuf); 

    label_cleanup: 
        free(info);
        gretchenTX_destroy(tx);
        grtModemOpt_destroy(opt);

    return 0;
}
