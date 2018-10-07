#include "gretchen.internal.h"

// since i get a clang warning: `Variable 'opt' may be uninitialized when
// used here` and since clang is known to generate false positives i
// just added a pragma to ignore these warnings here in this file.
// see: https://stackoverflow.com/questions/52194930
#pragma clang diagnostic ignored "-Wconditional-uninitialized"
// i have to see whether this line should get into libgretchen or not.

static float _convert_freq2rad(uint32_t frequency, uint32_t samplerate)
{
    return ((float)frequency/(float)samplerate) * M_PI * 2.0;
}

static grtModemOpt_t* _create_empty(uint32_t samplerate)
{
    grtModemOpt_t *opt = calloc(1, sizeof(grtModemOpt_t));
    grtFrameOpt_t *frame = calloc(1, sizeof(grtFrameOpt_t));
    grtModulatorOpt_t *mod = calloc(1, sizeof(grtModulatorOpt_t));
    grtOfdmOpt_t *ofdm = calloc(1, sizeof(grtOfdmOpt_t));
    opt->ofdmopt = ofdm;
    opt->frameopt = frame;
    opt->modopt = mod;
    opt->samplerate = samplerate;
    // other defaults
    mod->flushlen_mod = 4;
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

static bool _are_all_values_set(grtModemOpt_t* opt)
{
    if (opt->frametype == frametype_unset)
        return false;
    if (opt->frametype == frametype_ofdm)
        if (opt->ofdmopt->num_subcarriers == 0 ||
            opt->ofdmopt->cyclic_prefix_len == 0 ||               
            opt->ofdmopt->taper_len == 0 ||      
            opt->ofdmopt->left_band == 0 ||
            opt->ofdmopt->right_band == 0)
            return false;
    if (opt->frameopt->payload_len == 0 ||
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

grtModemOpt_t* grtModemOpt_create_default(uint32_t samplerate)
{
    grtModemOpt_t* opt = _create_empty(samplerate);
    opt->frametype = frametype_modem;
    opt->frameopt->payload_len = 800;
    opt->frameopt->checksum_scheme = liquid_getopt_str2crc("crc32");
    opt->frameopt->inner_fec_scheme = liquid_getopt_str2fec("secded7264");
    opt->frameopt->outer_fec_scheme = liquid_getopt_str2fec("h84");
    opt->frameopt->mod_scheme = liquid_getopt_str2mod("qpsk");
    opt->frameopt->_bits_per_symbol = modulation_types[opt->frameopt->mod_scheme].bps;
    opt->modopt->shape = liquid_getopt_str2firfilt("pm");
    opt->modopt->samples_per_symbol = 9;
    opt->modopt->symbol_delay = 5;
    opt->modopt->excess_bw = 0.75;
    opt->modopt->center_rads = _convert_freq2rad(16200, samplerate);
    opt->modopt->gain = 0.45;
    return opt;
}

void grtModemOpt_destroy(grtModemOpt_t* opt)
{
    if (opt) {
        if (opt->frameopt)
            free(opt->frameopt);
        if (opt->modopt)
            free(opt->modopt);
        if (opt->ofdmopt)
            free(opt->ofdmopt);
        free(opt);
    }
}

#define TOKENLIST_CREATE() \
    uint32_t num_token = 0; \
    char** res_tokens  = NULL; \

#define TOKENLIST_ADD(token, len) \
    num_token++; \
    res_tokens = realloc(res_tokens, sizeof(char*)*num_token); \
    if (res_tokens==NULL) \
        goto tokenlist_free; \
    res_tokens[num_token-1] = malloc(sizeof(char)*len+1); \
    strncpy(res_tokens[num_token-1], token, len+1); \

#define TOKENLIST_DESTROY() \
    tokenlist_free: \
        for(uint32_t i = 0; i < num_token; i++) \
            free(res_tokens[i]); \
        free(res_tokens); \

grtModemOpt_t* grtModemOpt_parse_args_from_file(uint8_t* filename, bool is_tx, uint32_t samplerate)
{
    // 1 load the options file
    int8_t error;
    int64_t filesize;
    uint8_t* optchar = read_binaryfile(filename, &filesize, &error);
    if (error!=0) {
        printf("cannot read opt file.\n");
        return NULL;
    }
    // 2 split the lines of the file
    TOKENLIST_CREATE();
    // at 0 add the program name (here i just put none into it)
    TOKENLIST_ADD("\0", 1);
    // FIXME 
    // i have to add these two in order to work with parse_args... oO why??
    TOKENLIST_ADD("\0",1);
    TOKENLIST_ADD("\0",1);
    TOKENLIST_ADD("\0",1);
    TOKENLIST_ADD("\0",1);
    // walk all tokens (lines of the options data)
    for (char* p=strtok((char*)optchar,"\n"); p!=NULL; p=strtok(NULL,"\n")) {
        // copy the token first
        char* dup = strdup(p);
        // find the pointer to the ' ' char of the token
        const char* pidx = strchr(dup, ' ');
        if (pidx) {
            // get the actual index of the pointer
            size_t index = pidx - dup;
            // substring arg
            size_t lenA = index;
            char arg[lenA+1];
            memcpy(arg, &dup[0], lenA);
            arg[lenA] = '\0';
            // substring val
            size_t lenB = strlen(dup)-index-1;
            char val[lenB];
            memcpy(val, &dup[lenA+1], lenB);
            val[lenB] = '\0';
            /*printf("%s %s\n",arg, val);*/
            // add first token
            TOKENLIST_ADD(arg, strlen(arg));
            // add second token
            TOKENLIST_ADD(val, strlen(val));
        }
        free(dup);
    }
    // 3 feed the char** tokenlist into grtModemOpt
    grtModemOpt_t* opt = grtModemOpt_parse_args(num_token, res_tokens, is_tx, samplerate);    
    TOKENLIST_DESTROY();
    free(optchar);
    return opt;
}

grtModemOpt_t* grtModemOpt_parse_args(int argc, char** argv, bool is_tx, uint32_t samplerate) 
{
    if (argc==1)
        return NULL;
    
    grtModemOpt_t* opt = _create_empty(samplerate);
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
        
        {"ofdmnsub", required_argument, NULL, '6'},
        {"ofdmprefix", required_argument, NULL, '7'},
        {"ofdmtaper", required_argument, NULL, '8'},
        {"ofdmlband", required_argument, NULL, '9'},
        {"ofdmrband", required_argument, NULL, 'a'},

        {NULL, 0, NULL, 0}
    };

    int ch;
    while ((ch = getopt_long(argc, argv, 
                             "0:1:2:3:4:5:b:c:d:e:f:g:h:i:j:k:l:m:n:6:7:8:9:a:", 
                             long_options, NULL)) != -1) {
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
                opt->frameopt->payload_len = atoi(optarg);
                if (opt->frameopt->payload_len == 0) {
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
                    opt->modopt->center_rads = _convert_freq2rad(atoi(optarg), samplerate);
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
            // OFDM options
            case '6':
                opt->ofdmopt->num_subcarriers = atoi(optarg);
                if (opt->ofdmopt->num_subcarriers == 0) {
                    printf("arg: error ofdm num subcarriers == 0!\n");
                    inputvalid = false;
                }
                break;
            case '7':
                opt->ofdmopt->cyclic_prefix_len = atoi(optarg);
                if (opt->ofdmopt->cyclic_prefix_len == 0) {
                    printf("arg: error ofdm cyclic prefix len == 0!\n");
                    inputvalid = false;
                }
                break;
            case '8':
                opt->ofdmopt->taper_len = atoi(optarg);
                if (opt->ofdmopt->taper_len == 0) {
                    printf("arg: error ofdm taper len == 0!\n");
                    inputvalid = false;
                }
                break;
            case '9':
                opt->ofdmopt->left_band = atoi(optarg);
                if (opt->ofdmopt->left_band == 0) {
                    printf("arg: error ofdm left band == 0!\n");
                    inputvalid = false;
                }
                break;
            case 'a':
                opt->ofdmopt->right_band = atoi(optarg);
                if (opt->ofdmopt->right_band == 0) {
                    printf("arg: error ofdm right band == 0!\n");
                    inputvalid = false;
                }
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
    printf("samplerate %u \n", opt->samplerate);
    printf("frametype %u \n", opt->frametype);
    printf("framelen %zu \n", opt->frameopt->payload_len);
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
    printf("ofdmnsub %u \n", opt->ofdmopt->num_subcarriers);
    printf("ofdmprefix %u \n", opt->ofdmopt->cyclic_prefix_len);
    printf("ofdmtaper %u \n", opt->ofdmopt->taper_len);
    printf("ofdmlband %zu \n", opt->ofdmopt->left_band);
    printf("ofdmrband %zu \n", opt->ofdmopt->right_band);
    printf("\n");
}

