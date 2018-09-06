#include <stdio.h>
#include <stdlib.h>

#include "gretchen.h"

void progress_callback(
                unsigned long hash,
                unsigned int frame_num,
                unsigned int frame_nummax,
                int payload_valid, 
                void* user) {
    (void) user;
    printf("rx progress callback: hash %lu num %i max %i payloadvalid %i\n", 
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

    uint8_t *path = "test/\0";
    uint8_t *name = malloc(sizeof(uint8_t)*(strlen(path)+strlen(filename))+2);
    strcpy(name, path);
    // FIXME this filesystemdelimiterstuff is hardly platform independent
    // solve or factor out
    /*strcat(name, "_");*/
    // i could require that path ends with '/' or (see above) legel delim
    strcat(name, filename);
    int error;
    write_binary_file(name, source, &error);
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

    int error;
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



