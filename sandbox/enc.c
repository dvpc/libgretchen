#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "gretchen.internal.h"


void emit_callback(size_t buffer_len, float *buffer, void *userdata)
{
    (void) userdata;
    /*fprintf(stderr,"emit buf: %zu len %p \n", buffer_len, buffer);*/
    if (buffer_len > 0) {
        fwrite(buffer, sizeof(float), buffer_len, stdout);
    }
}

int main(int argc, char **argv) {

    FILE *input = stdin;
    FILE *output = stdout;

    grtModemOpt_t* opt = grtModemOpt_parse_args(argc, argv, true, 96000); 
    if (!opt)
        return -1;


    // FIXME
    // if framelen < 150 it will crash
    grtModemTX_t *enc = grtModemTX_create(opt, 1 << 14);
    enc->emit_callback = emit_callback;
    enc->emit_callback_userdata = NULL;
    // dummy values. 
    // can't make a hash from the whole file since we just see chunks in this here
    // also can't estimate filesize here... so setting about 2K
    grtModemTX_setheaderinfo(enc, 11376, 2048);

    // FIXME check buffer sizes!!!
    // also read all data!!!
    size_t readbuf_len = 1 << 10;
    uint8_t* readbuf = malloc(readbuf_len * sizeof(uint8_t));


    // FIXME
    // can i move this away from the user??? API?
    size_t frame_len = 1 << 8;
   
    // FIXME rewrite all of this... 
    while(true) {
        size_t nread = fread(readbuf,
                             sizeof(uint8_t), 
                             readbuf_len, 
                             input);
        if (nread == 0) {
            // FIXME needs to be called like the following
            // if its just internal that might be ok...
            grtModemTX_enable_flush(enc);
            grtModemTX_consume(enc, readbuf, 0);
        } else { 
            size_t chnklen; 
            size_t idx = 0;
            while(true) {
                if (idx+frame_len > nread)
                    chnklen = nread - idx;
                else
                    chnklen = frame_len;
                size_t consumed = grtModemTX_consume(enc, readbuf+idx, chnklen);
                if (consumed!=chnklen) {
                    if (consumed==0)
                        break;
                    fprintf(stderr,"___enc: consume error: loosing %zu bytes\n",
                                chnklen-consumed);
                }
                idx += consumed;
                if (idx >= nread)
                    break;
            }
        }

        if (nread == 0)
            break;
    }
    /*usleep(1000000);*/
    /*fprintf(stderr,"Done\n");*/

    free(readbuf);
    fclose(output);
    fclose(input);
    grtModemTX_destroy(enc);
    grtModemOpt_destroy(opt);
    return 0;
}
