#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "gretchen.h"
#include "gretchen.backend.h"
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

static void print_usage();
static void print_banner();
static void rxprogress_callback();
static void rxfilecomplete_callback();


// if listening (rx) or transmitting (tx).
static bool is_tx = false;
static uint8_t* txfilepath = NULL;
// if using default options.
static bool use_defaultoption = true;
static uint8_t* optionfilepath = NULL;
// directory path for storing incoming files.
static uint8_t* outpath = "incoming/\0";


int main(int argc, char **argv) {

    // parse the command line args
    char c;
    while(1) {
        c = getopt(argc, argv, "f:o:p:h:"); 
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
            case 'p':
                // if path contains a trailing slash 
                if (optarg[strlen(optarg)-1]=='/')
                    outpath = optarg;
                else
                    printf("   Warning. Missing trailing slash in\n");
                    printf("   output dirname. Using default %s.\n", outpath);
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
        opt = grtModemOpt_create_default(48000);
    } else {
        opt = grtModemOpt_parse_args_from_file(optionfilepath, is_tx, 48000);
    }
    if (!opt)
        goto cleanup_opt;

    // setup audio backend
    size_t internbuflen = 1 << 14;
    grtBackend_t* back = grtBackend_create(internbuflen, is_tx, 48000);
    if (back==NULL) {
        fprintf(stderr, ".. Error cannot initialize audio backend.\n");
        goto cleanup_backend;
    }
    print_banner();

    // setup gretchen 
    void *modem;
    if (is_tx) {
        printf(".. TX (transfer) mode\n");
        modem = (gretchenTX_t*) gretchenTX_create(opt, 1<<13);//8192

        // load file from txfilepath
        gretchenTX_inspect_t* info;
        int8_t error;
        gretchenTX_inspect(modem, txfilepath, &error, &info);
        if (info==NULL || error!=0) {
            fprintf(stderr, ".. Error cannot process file. %s\n", argv[1]);
            goto cleanup_modem;
        }
        printf("   filesize (bytes) %zu\n", info->filesize_bytes);
        printf("   estimated time (sec) %zu\n", info->est_transfer_sec);
        free(info); 

        // encode the file
        gretchenTX_prepare(modem, txfilepath, &error);
        // get the sample
        float* samplebuf;
        size_t samplebuflen;
        gretchenTX_get(modem, &samplebuf, &samplebuflen);

        // playback the sample
        grtBackend_startstream(back, &error);
        if (error != 0) {
            fprintf(stderr, ".. Error backend cannot start stream.\n");
            goto cleanup_modem;
        }
        size_t buflen = 1<<13;//8192
        size_t avail, len, pushed;
        size_t k = 0;
        bool done = false;
        while(!done) {
            avail = grtBackend_push_available(back);
            len = avail < buflen ? avail : buflen;
            if (k+len>samplebuflen) {
                len = samplebuflen-k;
                done = true;
            }
            printf("\r.. %.3f %%     ", (float)k/(float)samplebuflen*100);
            fflush(stdout); 
            pushed = grtBackend_push(back, samplebuf+k, len);
            if (len!=pushed)
                fprintf(stderr, ".. Error backend loosing %zu samples.", len-pushed);
            k += len;
            Pa_Sleep(150);
        }
        printf("\n");
        // cleanup tx
        free(samplebuf);
        grtBackend_stopstream(back, &error);

    } else {
        printf(".. RX (receive) mode\n");
        modem = (gretchenRX_t*) gretchenRX_create(opt, 1<<14);//16384
        ((gretchenRX_t*) modem)->callback = rxfilecomplete_callback;
        ((gretchenRX_t*) modem)->prog_callback = rxprogress_callback; 
        ((gretchenRX_t*) modem)->callbackuser = modem;

        // start recording
        int8_t error;
        grtBackend_startstream(back, &error);
        if (error != 0) {
            fprintf(stderr, ".. Error backend cannot start stream. \n");
            goto cleanup_modem;
        }
        grtSigcatcher_Init();
        size_t asklen = 1<<14;//8192
        float* buffer = NULL;
        size_t nread;
        uint32_t num_channels = back->strParams.channelCount;
        float* monobuf = malloc(sizeof(float)*asklen);
        while(!grtSigcatcher_ShouldTerminate()) {
            if (!grtBackend_isstreamactive(back)) {
                fprintf(stderr, ".. Recording stream stopped. %s \n", 
                                grtBackend_getstatustext(back));
                grtSigcatcher_Set(1);
                break;
            }
            grtBackend_poll(back, asklen, &buffer, &nread);
            /*printf("ask %zu nread %zu buff %p \n", asklen, nread, buffer);*/
            if (buffer!=NULL && nread>0) {
                size_t idx=0;
                for (size_t k=0; k<nread; k+=num_channels) {
                    monobuf[idx] = buffer[k];
                    idx++;
                }
                gretchenRX_push_le16f(modem, monobuf, idx, &error);
                if (error!=0) {
                    fprintf(stderr, ".. Error modem overflow. \n");
                    break;
                }
            }
            Pa_Sleep(150); 
        } 
        printf("\n");
        // cleanup rx
        free(monobuf);
        grtSigcatcher_Destroy();
        grtBackend_stopstream(back, &error);
    }

cleanup_modem:
        if (is_tx)
            gretchenTX_destroy(modem);
        else
            gretchenRX_destroy(modem);

cleanup_backend:
        grtBackend_destroy(back);

cleanup_opt:
        grtModemOpt_destroy(opt);

    return 0;
}



static void print_usage(uint8_t* binname) {
    print_banner();
    printf(".. Usage: %s \n", binname);
    printf("   -f <file to transmit>\n");
    printf("   -o <modem option (file)>\n");
    printf("   -p <output path>.\n\n");
}

static void print_banner() {
    printf("\n.. Gretchen version: %i.%i\n", 
                    gretchen_VERSION_MAJOR, 
                    gretchen_VERSION_MINOR);
}

static void print_transm(transmit_t* t, void* user) {

    (void) user;
    printf("[hash %lu chnks ", t->hash);
    for (uint32_t k=0; k<t->max; k++) {
        printf("%s", t->chunks[k].data==NULL?".":"O"); 
    }
    printf("] ");
    fflush(stdout);
}

static uint32_t print_count = 0;
static void rxprogress_callback(
                uint64_t hash,
                uint32_t frame_num,
                uint32_t frame_nummax,
                int payload_valid, 
                void* user) {
    (void) hash;
    (void) frame_num; 
    (void) frame_nummax;
    (void) payload_valid;
    gretchenRX_t* rx = (gretchenRX_t*)user;
    uint8_t* prgr;
    switch(print_count%8) {
        case 0: prgr = ". \0"; break; 
        case 1: prgr = "..\0"; break; 
        case 2: prgr = ":.\0"; break; 
        case 3: prgr = "::\0"; break; 
        case 4: prgr = ".:\0"; break; 
        case 5: prgr = "..\0"; break; 
        case 6: prgr = " .\0"; break; 
        default:prgr = "  \0";
    }
    printf("\r%s ", prgr);
    fflush(stdout);
    rxhandler_list(rx->rxhandler, print_transm, NULL);
    /*printf("\n");*/
    /*printf("rx progress callback: hash %lu num %i max %i payloadvalid %i\n", */
                    /*hash, frame_num, frame_nummax, payload_valid);*/
    print_count ++;
}

static void rxfilecomplete_callback(
                uint8_t* filename, 
                uint8_t* source, 
                size_t sourcelen, 
                void* user) {
    (void) source;
    (void) user;
    gretchenRX_t* rx = (gretchenRX_t*)user;
    printf("\rok ");
    rxhandler_list(rx->rxhandler, print_transm, NULL);
    printf("\n");
    printf("   File complete: %s size (bytes) %zu \n", filename, sourcelen);

    // FIXME this filesystemdelimiterstuff is hardly platform independent
    // solve or factor out
    // seems windows has sth https://stackoverflow.com/questions/9235679/create-a-directory-if-it-doesnt-exist#9235708
    int direrror;
    DIR* dir = opendir(outpath);
    if (dir) {
        closedir(dir);
        direrror = 0;
    } else if (ENOENT==errno) {
        direrror = mkdir(outpath, 0777);
    } else {
        direrror = 1; 
    }

    uint8_t *name = malloc(sizeof(uint8_t)*(strlen(outpath)+strlen(filename))+2);
    strcpy(name, outpath);
    strcat(name, filename);

    int8_t error;
    write_binaryfile(name, source, &error);
    printf("   File written with %s (%i)\n", error?"error":"no error", error);
    free(name);
}

