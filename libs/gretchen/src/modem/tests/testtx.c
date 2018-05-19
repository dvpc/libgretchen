#include <stdio.h>
#include <stdlib.h>

#include "gretchen.h"

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
    // FIXME 
    // important i have to set this by hand here!!!
    // think of a method....
    opt->frameopt->_bits_per_symbol = modulation_types[opt->frameopt->mod_scheme].bps;
    opt->modopt->shape = liquid_getopt_str2firfilt("pm");
    opt->modopt->samples_per_symbol = 9;
    opt->modopt->symbol_delay = 5;
    opt->modopt->excess_bw = 0.75;
    opt->modopt->center_rads = grtModemOpt_convert_freq2rad(16200, 44100);
    opt->modopt->gain = 0.45;

    // TODO buffersizes... and internal chunk sizes...
    gretchenTX_t* tx = gretchenTX_create(opt, 1<<12);
    int error;

    gretchenTX_inspect_t* info;
    gretchenTX_inspect(tx, argv[1], &error, &info);
    if (info==NULL)
        goto label_cleanup;
    fprintf(stderr, "\n");
    fprintf(stderr, "info file: toolarge %i bytes %zu est-samples: %zu est-time-sec: %zu\n", 
                    info->is_toolarge, info->filesize_bytes, info->est_encodedsize_samples, info->est_transfer_sec);


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
