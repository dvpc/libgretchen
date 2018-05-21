#include "gretchen.internal.h"


float grtModemOpt_convert_freq2rad(int frequency, int samplerate) 
{
    return ((float)frequency/(float)samplerate) * M_PI * 2.0;
}

grtModemOpt_t* grtModemOpt_create_empty()
{
    grtModemOpt_t *opt = calloc(1, sizeof(grtModemOpt_t));
    grtFrameOpt_t *frame = calloc(1, sizeof(grtFrameOpt_t));
    grtModulatorOpt_t *mod = calloc(1, sizeof(grtModulatorOpt_t));
    opt->frameopt = frame;
    opt->modopt = mod;
    // other defaults
    mod->flushlen_mod = 5;
    frame->_bits_per_symbol = 1;
    // filter defaults
    mod->txflt_order = 6;
    mod->txflt_cutoff_frq = .25f;
    mod->txflt_center_frq = .45f;
    mod->rxflt_order = 7;
    mod->rxflt_cutoff_frq = .3f;
    mod->rxflt_center_frq = .36f;
    return opt;
}

void grtModemOpt_destroy(grtModemOpt_t* opt)
{
    if (opt) {
        if (opt->frameopt)
            free(opt->frameopt);
        if (opt->modopt)
            free(opt->modopt);
        free(opt);
    }
}


static bool _are_all_values_set(grtModemOpt_t* opt)
{
    if (opt->frametype == frametype_unset)
        return false;
    if (opt->frameopt->frame_len == 0 ||
        opt->frameopt->checksum_scheme == 0 ||
        opt->frameopt->inner_fec_scheme == 0 ||
        opt->frameopt->outer_fec_scheme == 0 ||
        opt->frameopt->mod_scheme == 0) 
        return false;
    if (opt->modopt->shape == 0 ||
        opt->modopt->samples_per_symbol == 0 ||
        opt->modopt->symbol_delay == 0 ||
        opt->modopt->excess_bw == 0 ||
        opt->modopt->center_rads == 0 ||
        opt->modopt->gain == 0)
        return false;
    // expelling values with defaults to be checked. see above.
    return true;
}

grtModemOpt_t* grtModemOpt_parse_args(int argc, char** argv, bool is_tx) 
{
    if (argc==1)
        return NULL;
    
    grtModemOpt_t* opt = grtModemOpt_create_empty();
    bool inputvalid = true; 

    static struct option long_options[] =
    {
        {"frmtype", required_argument, NULL, '0'},
        {"frmlen", required_argument, NULL, '1'},
        {"frmcrc", required_argument, NULL, '2'},
        {"frmifec", required_argument, NULL, '3'},
        {"frmofec", required_argument, NULL, '4'},
        {"frmmod", required_argument, NULL, '5'},
        {"modshape", required_argument, NULL, 'b'},
        {"modsampsym", required_argument, NULL, 'c'},
        {"modsymdelay", required_argument, NULL, 'd'},
        {"modexcbandw", required_argument, NULL, 'e'},
        {"modfreq", required_argument, NULL, 'f'},
        {"modgain", required_argument, NULL, 'g'},

        {"txorder", required_argument, NULL, 'h'},
        {"txcutoff", required_argument, NULL, 'i'},
        {"txcenter", required_argument, NULL, 'j'},
        {"rxorder", required_argument, NULL, 'k'},
        {"rxcutoff", required_argument, NULL, 'l'},
        {"rxcenter", required_argument, NULL, 'm'},

        {"flushlen", required_argument, NULL, 'n'},
        {NULL, 0, NULL, 0}
    };
    int ch;
    while ((ch = getopt_long(argc, argv, 
                             "0:1:2:3:4:5:b:c:d:e:f:g:h:i:j:k:l:m:n:", 
                             long_options, NULL)) != -1) {
        // FIXME remove
        printf("ch %c optarg %s\n", ch, optarg);
        switch (ch) {
            case '0':
                if (strncmp(optarg, "ofdm", 5)==0)
                    opt->frametype = frametype_ofdm;
                else if (strncmp(optarg, "modem", 5)==0)
                    opt->frametype = frametype_modem;
                else if (strncmp(optarg, "gmsk", 5)==0)
                    opt->frametype = frametype_gmsk;
                else {
                    printf("arg: error frametype not recognized!\n");
                    inputvalid = false;
                }
                break;
            case '1':
                opt->frameopt->frame_len = atoi(optarg);
                if (opt->frameopt->frame_len == 0) {
                    printf("arg: error framelen == 0!\n");
                    inputvalid = false;
                }
                break;
            case '2':
                opt->frameopt->checksum_scheme = liquid_getopt_str2crc(optarg);
                if (opt->frameopt->checksum_scheme == LIQUID_CRC_UNKNOWN) {
                    printf("arg: error frame crc scheme not recognized!\n");
                    inputvalid = false;
                }
                break;
            case '3':
                opt->frameopt->inner_fec_scheme = liquid_getopt_str2fec(optarg);
                if (opt->frameopt->inner_fec_scheme == LIQUID_FEC_UNKNOWN) {
                    printf("arg: error frame inner fec scheme not recognized!\n");
                    inputvalid = false;
                }
                break;
            case '4':
                opt->frameopt->outer_fec_scheme = liquid_getopt_str2fec(optarg);
                if (opt->frameopt->outer_fec_scheme == LIQUID_FEC_UNKNOWN) {
                    printf("arg: error frame outer fec scheme not recognized!\n");
                    inputvalid = false;
                }
                break;
            case '5':
                opt->frameopt->mod_scheme = liquid_getopt_str2mod(optarg);
                if (opt->frameopt->mod_scheme == LIQUID_MODEM_UNKNOWN) {
                    printf("arg: error frame modulation scheme not recognized!\n");
                    inputvalid = false;
                }
                // I also set bps `bits per symbol` (encoding depth)
                // depending on the modulation scheme
                // it is used only internally to estimate transmission time etc.. (see grt_tx)
                opt->frameopt->_bits_per_symbol = modulation_types[opt->frameopt->mod_scheme].bps;
                break;
            // MODULATION OPTIONS
            case 'b':
                if (strncmp(optarg, "gmsk", 5)==0) {
                    if (is_tx)
                        opt->modopt->shape = liquid_getopt_str2firfilt("gmsktx");
                    else
                        opt->modopt->shape = liquid_getopt_str2firfilt("gmskrx");
                } else { 
                    opt->modopt->shape = liquid_getopt_str2firfilt(optarg);
                }
                if (opt->modopt->shape == LIQUID_FIRFILT_UNKNOWN) {
                    printf("arg: error modulation shape not recognized!\n");
                    inputvalid = false;
                }
                break;
            case 'c':
                opt->modopt->samples_per_symbol = atoi(optarg);
                if (opt->modopt->samples_per_symbol == 0) {
                    printf("arg: error modulation samples per symbols == 0!\n");
                    inputvalid = false;
                }
                break;
            case 'd':
                opt->modopt->symbol_delay = atoi(optarg);
                if (opt->modopt->symbol_delay == 0) {
                    printf("arg: error modulation symbol delay == 0!\n");
                    inputvalid = false;
                }
                break;
            case 'e':
                opt->modopt->excess_bw = atof(optarg);
                if (opt->modopt->excess_bw == 0) {
                    printf("arg: error modulation excess bandwidth == 0!\n");
                    inputvalid = false;
                }
                break;
            case 'f':
                opt->modopt->center_rads = atof(optarg);
                if (opt->modopt->center_rads == 0) {
                    printf("arg: error modulation center frequency == 0!\n");
                    inputvalid = false;
                } else {
                    // FIXME parameter samplingrate??
                    opt->modopt->center_rads = grtModemOpt_convert_freq2rad(atoi(optarg), 44100);
                }
                break;
            case 'g':
                opt->modopt->gain = atof(optarg);
                if (opt->modopt->gain == 0) {
                    printf("arg: error modulation gain == 0!\n");
                    inputvalid = false;
                }
                break;
            // below values have defaults. no validity is checked.   
            case 'h':
                opt->modopt->txflt_order = atoi(optarg);
                break;
            case 'i':
                opt->modopt->txflt_cutoff_frq = atof(optarg);
                break;
            case 'j':
                opt->modopt->txflt_center_frq = atof(optarg);
                break;
            case 'k':
                opt->modopt->rxflt_order = atoi(optarg);
                break;
            case 'l':
                opt->modopt->rxflt_cutoff_frq = atof(optarg);
                break;
            case 'm':
                opt->modopt->rxflt_center_frq = atof(optarg);
                break;
            case 'n':
                opt->modopt->flushlen_mod = atof(optarg);
                break;
            default:
                // if any arg is not recognized all is false!
                inputvalid = false;
        }
    }

    bool ok = _are_all_values_set(opt);
    if (ok && inputvalid) {
        return opt;
    } else {
        printf("\nError! Incorrect options\n");
        grtModemOpt_print(opt);
        grtModemOpt_destroy(opt);
        return NULL;    
    }
}

void grtModemOpt_print(grtModemOpt_t* opt)
{
    if (!opt)
        return;
    bool ok = _are_all_values_set(opt);
    printf("are all values set?: %i\n", ok);
    printf("\n");
    printf("frametype %u \n", opt->frametype);
    printf("framelen %zu \n", opt->frameopt->frame_len);
    printf("framecrc %u \n", opt->frameopt->checksum_scheme);
    printf("frameifec %u \n", opt->frameopt->inner_fec_scheme);
    printf("frameofec %u \n", opt->frameopt->outer_fec_scheme);
    printf("framemod %u \n", opt->frameopt->mod_scheme);
    printf("modshape %u \n", opt->modopt->shape);
    printf("modsampsym %u \n", opt->modopt->samples_per_symbol);
    printf("modsymdelay %u \n", opt->modopt->symbol_delay);
    printf("modexcbandw %f \n", opt->modopt->excess_bw);
    printf("modfreq %f \n", opt->modopt->center_rads);
    printf("modgain %f \n", opt->modopt->gain);
    printf("\n");
}

