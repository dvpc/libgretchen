#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "gretchen.h"
#include "gretchen.backend.h"

static void print_usage(char* binname);


int main(int argc, char **argv) {
    // if listening (rx) or transmitting (tx)
    bool is_tx = false;
    char* txfilepath = NULL;
    // if using default options
    bool use_defaultoption = true;
    char* optionfilepath = NULL;
    // parse the command line args
    char c;
    while(1) {
        c = getopt(argc, argv, "f:o:h:"); 
        if (c==-1)
            break;
        switch(c) {
            case 'f':
                is_tx = true;
                txfilepath = optarg;
                break; 
            case 'o':
                use_defaultoption = false;
                optionfilepath = optarg;
                break; 
            case 'h':
                print_usage(argv[0]);
                return 0;
                break; 
        default:
            print_usage(argv[0]);
            return 0;
        }
    }
    // setup options
    grtModemOpt_t* opt = NULL; 
    if (use_defaultoption) {
        opt = grtModemOpt_create_empty();
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
        opt = grtModemOpt_parse_args_from_file(optionfilepath, is_tx);
    }
    // TODO will be removed
    /*if (opt)*/
        /*grtModemOpt_print(opt);*/
    // setup backend
    if (is_tx) {

    } else {
    
    }




    if (is_tx) {
        // load file from txfilepath
        //
        // etc...
    } else {
        // start listening mode etc...
    
    
    }


    grtModemOpt_destroy(opt);


    return 0;
}



static void print_usage(char* binname)
{
    printf("Gretchen version: %i.%i\nUsage: %s [[-f <file to transmit>] -o <modem options>].\n",
        gretchen_VERSION_MAJOR, gretchen_VERSION_MINOR, binname);
}
