#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "gretchen.h"
#include "gretchen.backend.h"

static void print_usage();
static void rxprogress_callback();
static void rxfilecomplete_callback();
static void debug_callback();


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
        opt = grtModemOpt_create_default();
    } else {
        opt = grtModemOpt_parse_args_from_file(optionfilepath, is_tx);
    }
    if (!opt)
        return 1;
    // setup audio backend
    size_t internbuflen = 1 << 14;
    grtBackend_t* back = grtBackend_create(internbuflen, is_tx);
    // NOTE this Pa_Sleep is needed if more than one backend 
    // (e.g. playback and record at the same time) is used.
    // FIXME investigate is it still needed??
    Pa_Sleep(150);
    void *modem;

    if (is_tx) {
        // setup modem 
        modem = (gretchenTX_t*) gretchenTX_create(opt, 1<<12);

        // load file from txfilepath
        gretchenTX_inspect_t* info;
        int error;
        gretchenTX_inspect(modem, txfilepath, &error, &info);
        if (info==NULL || error!=0) {
            printf("Gretchen error: Cannot process file. %s\n", argv[1]);
            goto cleanup;
        }
        // FIXME printing that info for now...
        printf("info file: bytes %zu est-samples: %zu est-time-sec: %zu\n\n", 
            info->filesize_bytes, info->est_encodedsize_samples, info->est_transfer_sec);
        free(info); 

        // encode the file
        gretchenTX_prepare(modem, txfilepath, &error);
        // get the sample
        float* samplebuf;
        size_t samplebuflen;
        gretchenTX_get(modem, &samplebuf, &samplebuflen);

        // play the sample
        grtBackend_startstream(back, &error);
        if (error != 0) {
            fprintf(stderr, "backend tx: cannot start stream.\n");
            goto cleanup;
        }
        size_t buflen = 8192;
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
            // FIXME 
            // progress for playing back the sample
            /*printf("avail %zu len %zu k %zu done %i \n", avail, len, k, done);*/
            pushed = grtBackend_push(back, samplebuf+k, len);
            if (len!=pushed)
                printf("backend (tx): loosing %zu samples.", len-pushed);
            k += len;
            Pa_Sleep(150);
        }

        // cleanup tx
        free(samplebuf);
        grtBackend_stopstream(back, &error);
    } else {
        // setup modem 
        modem = (gretchenRX_t*) gretchenRX_create(opt, 1<<14);
        ((gretchenRX_t*) modem)->callback = rxfilecomplete_callback;
        ((gretchenRX_t*) modem)->prog_callback = rxprogress_callback; 
        /*((gretchenRX_t*) modem)->modem_rx->emit_debug_callback = debug_callback;*/

        // start listening mode
        int error;
        grtBackend_startstream(back, &error);
        if (error != 0) {
            printf("backend (rx): cannot start stream. \n");
            goto cleanup;
        }
        grt_sigcatch_Init();
        size_t asklen = 8192;
        float* buffer = NULL;
        size_t nread;
        while(!grt_sigcatch_ShouldTerminate()) {
            if (!grtBackend_isstreamactive(back)) {
                grt_sigcatch_Set(1);
                break;
            }
            grtBackend_poll(back, asklen, &buffer, &nread);
            /*printf("ask %zu nread %zu buff %p \n", asklen, nread, buffer);*/
            if (buffer!=NULL && nread>0) {
                gretchenRX_push_le16f(modem, buffer, nread, &error);
                /*printf("err %i\n", error);    */
                // FIXME
                // error will be -1 if internal buffer size is too 
                // small versus asklen...
                // set them automatically!!!!
            }
            Pa_Sleep(150); 
        }
        // FIXME this will be a problem ...
        // since we just listen 
        // so flush the rest is bad here
        // ...
        gretchenRX_push_le16f(modem, buffer, 0, &error);

        // cleanup rx
        grt_sigcatch_Destroy();
        grtBackend_stopstream(back, &error);
    }

    cleanup:
        if (is_tx)
            gretchenTX_destroy(modem);
        else
            gretchenRX_destroy(modem);
        grtBackend_destroy(back);
        grtModemOpt_destroy(opt);

    return 0;
}



static void print_usage(char* binname)
{
    printf("Gretchen version: %i.%i\nUsage: %s [[-f <file to transmit>] -o <modem options>].\n",
        gretchen_VERSION_MAJOR, gretchen_VERSION_MINOR, binname);
}



static void rxprogress_callback(
                unsigned long hash,
                unsigned int frame_num,
                unsigned int frame_nummax,
                int payload_valid, 
                void* user) {
    (void) user;
    printf("rx progress callback: hash %lu num %i max %i payloadvalid %i\n", 
                    hash, frame_num, frame_nummax, payload_valid);
}


static void rxfilecomplete_callback(
                char* filename, 
                char* source, 
                size_t sourcelen, 
                void* user) {
    (void) source;
    (void) user;
    printf("rx file complete: name %s len %zu \n", 
                    filename, sourcelen);

    // FIXME this filesystemdelimiterstuff is hardly platform independent
    // solve or factor out
    /*strcat(name, "_");*/
    // i could require that path ends with '/' or (see above) legel delim
    char *path = "test/\0";
    char *name = malloc(sizeof(char)*(strlen(path)+strlen(filename))+2);
    strcpy(name, path);
    strcat(name, filename);
    int error;
    write_binary_file(name, source, &error);
    printf("File written with error %i \n", error);
    free(name);
}


static void debug_callback(int header_valid, int payload_valid, unsigned int payload_len, framesyncstats_s stats)
{
    fprintf(stderr, "__callback h-valid:%i p-valid:%i len:%i\n", header_valid, payload_valid, payload_len);
    fprintf(stderr, "    EVM                 :   %12.8f dB\n", stats.evm);
    fprintf(stderr, "    rssi                :   %12.8f dB\n", stats.rssi);
    fprintf(stderr, "    carrier offset      :   %12.8f Fs\n", stats.cfo);
    fprintf(stderr, "    num symbols         :   %u\n", stats.num_framesyms);
    fprintf(stderr, "    mod scheme          :   %s (%u bits/symbol)\n",
            modulation_types[stats.mod_scheme].name, stats.mod_bps);
    fprintf(stderr, "    validity check      :   %s\n", crc_scheme_str[stats.check][0]);
    fprintf(stderr, "    fec (inner)         :   %s\n", fec_scheme_str[stats.fec0][0]);
    fprintf(stderr, "    fec (outer)         :   %s\n", fec_scheme_str[stats.fec1][0]);
    fprintf(stderr, "    header crc          :   %s\n", header_valid ?  "pass" : "FAIL");
    fprintf(stderr, "    payload length      :   %u\n", payload_len);
    fprintf(stderr, "    payload crc         :   %s\n", payload_valid ?  "pass" : "FAIL");
}
