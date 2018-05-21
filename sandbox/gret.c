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

    grtModemOpt_t* opt = NULL; 
    if (use_defaultopt) {
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
        // 1 load the file
        int error;
        long filesize;
        char* optchar = read_binary_file(optpath, &filesize, &error);
        if (error!=0) {
            printf("cannot read opt file.\n");
            return -1;
        }

        // 2 split the lines
        int num_token = 0;
        char** res  = NULL;

        // at 0 add the program name (here i just put none into it)
        num_token++;
        res = realloc (res, sizeof (char*) * num_token);
        if (res==NULL)
            goto cleanup;
        res[num_token-1] = malloc(sizeof(char)*5);
        strcpy(res[num_token-1],"none\0");
        /*res[num_token-1] = malloc(sizeof(char)*strlen(&*argv[0]));*/
        /*strcpy(res[num_token-1],&*argv[0]);*/
        
        // walk all tokens (lines of the options data)
        for (char* p=strtok(optchar,"\n"); p!=NULL; p=strtok(NULL,"\n")) {
            // copy the token first
            char* dup = strdup(p);
            // find the pointer to the ' ' char of the token
            const char* pidx = strchr(dup, ' ');
            if (pidx) {
                // get the actual index of the pointer
                int index = pidx - dup;

                // substring arg
                int lenA = index;
                char arg[lenA+1];
                memcpy(arg, &dup[0], lenA);
                arg[lenA] = '\0';
                // substring val
                int lenB = strlen(dup)-index;
                char val[lenB];
                memcpy(val, &dup[lenA+1], lenB);
                val[lenB] = '\0';

                printf("%s %s\n",arg, val);

                // add first token
                num_token++;
                res = realloc (res, sizeof (char*) * num_token);
                if (res==NULL)
                    goto cleanup;
                res[num_token-1] = malloc(sizeof(char)*strlen(arg)+1);
                strcpy(res[num_token-1],arg);
                // add second token
                num_token++;
                res = realloc (res, sizeof (char*) * num_token);
                if (res==NULL)
                    goto cleanup;
                res[num_token-1] = malloc(sizeof(char)*strlen(val)+1);
                strcpy(res[num_token-1],val);
            }
            free(dup);
        }
        // 3 feed the char** into grtModemOpt
        opt = grtModemOpt_parse_args(num_token, res, is_modetx); 
        grtModemOpt_print(opt);
        
        cleanup:
            for(int i = 0; i < num_token; i++)
                free(res[i]);
            free(res);
            free(optchar);
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
