#include <stdio.h>
#include <stdlib.h>

#include "gretchen.h"

void progress_callback(
                unsigned long hash,
                unsigned int frame_num,
                unsigned int frame_nummax,
                void* user) {
    (void) user;
    printf("rx progress callback: hash %lu num %i max %i\n", 
                    hash, frame_num, frame_nummax);
}


void filecomplete_callback(
                char* filename, 
                char* source, 
                size_t sourcelen, 
                void* user) {
    (void) source;
    (void) user;
    printf("rx file complete: name %s len %zu \n", 
                    filename, sourcelen);

    char *path = "test/\0";
    char *name = malloc(sizeof(char)*(strlen(path)+strlen(filename))+2);
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

    grtModemOpt_t* opt = grtModemOpt_create_empty();
    opt->frametype = frametype_modem;
    opt->frameopt->frame_len = 800;
    opt->frameopt->checksum_scheme = liquid_getopt_str2crc("crc32");
    opt->frameopt->inner_fec_scheme = liquid_getopt_str2fec("secded7264");
    opt->frameopt->outer_fec_scheme = liquid_getopt_str2fec("h84");
    opt->frameopt->mod_scheme = liquid_getopt_str2mod("qpsk");
    opt->modopt->shape = liquid_getopt_str2firfilt("pm");
    opt->modopt->samples_per_symbol = 9;
    opt->modopt->symbol_delay = 5;
    opt->modopt->excess_bw = 0.75;
    opt->modopt->center_rads = grtModemOpt_convert_freq2rad(16200, 44100);
    opt->modopt->gain = 0.45;

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
        gretchenRX_push(rx, chunkbuf, rlen, &error);
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



