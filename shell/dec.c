/*
 * Gretchen dec
 *
 * Copyright (c) 2018 - 2019 Daniel von Poschinger
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "gretchen.internal.h"

void emit_callback(uint16_t hash, uint16_t header_num, uint16_t header_nummax, size_t buffer_len, uint8_t *buffer, void *userdata)
{
    (void) userdata;
    fprintf(stderr,"_dec_emit header-id: %hu frame-num %hu max %hu\n", hash, header_num, header_nummax);
    if (buffer_len > 0) {
        fwrite(buffer, sizeof(uint8_t), buffer_len, stdout);
    }
}

void debug_callback(int header_valid, int payload_valid, uint32_t payload_len, framesyncstats_s stats)
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

int main(int argc, char **argv) {

    FILE *input = stdin;
    FILE *output = stdout;

    grtModemOpt_t* opt = grtModemOpt_parse_args(argc, argv, false, 48000); 
    if (!opt)
        return -1;

    grtModemRX_t *dec = grtModemRX_create(opt, 1 << 16);
    dec->emit_callback = emit_callback;
    dec->emit_callback_userdata = NULL;
    dec->emit_debug_callback = debug_callback;

    size_t wantread = 1 << 12;
    float *samplebuf = malloc(wantread * sizeof(float));

    // FIXME
    // can i move this away from the user??? API?
    size_t frame_len = 1 << 10;

    while (true) {
        size_t nread = fread(samplebuf, sizeof(float), wantread, input);

        if (nread == 0) {
            // FIXME needs to be called like the following
            // if its just internal that might be ok...
            grtModemRX_enable_flush(dec);
            grtModemRX_consume(dec, samplebuf, 0);
        } else { 
            size_t chnklen; 
            size_t idx = 0;
            while(true) {
                if (idx+frame_len > nread)
                    chnklen = nread - idx;
                else
                    chnklen = frame_len;
                size_t consumed = grtModemRX_consume(dec, samplebuf+idx, chnklen);
                if (consumed!=chnklen) {
                    /*if (consumed==0)*/
                        /*break;*/
                    fprintf(stderr,"___dec: consume error: loosing %zu bytes\n",
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

    free(samplebuf);
    grtModemRX_destroy(dec);
    grtModemOpt_destroy(opt);
    fclose(output);
    fclose(input);
    return 0;
}
