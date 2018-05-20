#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "gretchen.h"
#include "gretchen.backend.h"

static void print_usage(char* binname)
{
    printf("Gretchen version: %i.%i\nUsage: %s [[-f <file to transmit>] -o <modem options>].\n",
        gretchen_VERSION_MAJOR, gretchen_VERSION_MINOR, binname);
}

int main(int argc, char **argv) {
    // if listening (rx) or transmitting (tx)
    bool is_modetx = false;
    // if using default options
    bool use_defaultopt = true;
    char* txfilepath = NULL;
    char* optpath = NULL;

    char c;
    while(1) {
        c = getopt(argc, argv, "f:o:h:"); 
        if (c==-1)
            break;
        switch(c) {
            case 'f':
                is_modetx = true;
                txfilepath = optarg;
                break; 
            case 'o':
                use_defaultopt = false;
                optpath = optarg;
                break; 
            case 'h':
                print_usage(argv[0]);
                break; 
        default:
            print_usage(argv[0]);
        }
    }

    grtModemOpt_t* opt = grtModemOpt_create_empty();
    if (use_defaultopt) {
        opt->frametype = frametype_modem;
        opt->frameopt->frame_len = 800;
        opt->frameopt->checksum_scheme = liquid_getopt_str2crc("crc32");
        opt->frameopt->inner_fec_scheme = liquid_getopt_str2fec("secded7264");
        opt->frameopt->outer_fec_scheme = liquid_getopt_str2fec("h84");
        opt->frameopt->mod_scheme = liquid_getopt_str2mod("qpsk");
        opt->frameopt->_bits_per_symbol = modulation_types[opt->frameopt->mod_scheme].bps;
        opt->modopt->shape = liquid_getopt_str2firfilt("pm");
        opt->modopt->samples_per_symbol = 9;
        opt->modopt->symbol_delay = 5;
        opt->modopt->excess_bw = 0.75;
        opt->modopt->center_rads = grtModemOpt_convert_freq2rad(16200, 44100);
        opt->modopt->gain = 0.45;
    } else {
        // TODO
        // load the opt file
        // create the options from it... 
        printf("-o parameter. Not implemented yet\n");
        return 1;
    }



    if (is_modetx) {
        // load file from txfilepath
        //
        // etc...
    } else {
        // start listening mode etc...
    
    
    }


    grtModemOpt_destroy(opt);


    return 0;
}
