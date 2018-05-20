#include "gretchen.internal.h"

static int dec_framesync_callback(
                unsigned char *header, 
                int header_valid, 
                unsigned char *payload,
                unsigned int payload_len, 
                int payload_valid,
                framesyncstats_s _stats,
                void *dvoid)
{
    (void) header;
    (void) _stats;
    if (!dvoid)
        return 0;
    grtModemRX_t *dec = dvoid;
    if (dec->emit_dubug_callback)
        dec->emit_dubug_callback(header_valid,
                                 payload_valid, 
                                 payload_len,
                                 _stats);
    if (!header_valid)
        return 1;
    if (!payload_valid)
        return 1;
    if (dec->emit_callback) {
        unsigned long hash; 
        unsigned int frame_num, frame_nummax;
        sscanf((char *)header,
               MODEM_HEADER_FORMAT, 
               &hash, 
               &frame_num,
               &frame_nummax);
        dec->emit_callback(hash,
                           frame_num,
                           frame_nummax,
                           payload_len, 
                           payload, 
                           dec->emit_callback_userdata);
    }
    return 0;
}

static void dec_modem_create(
                grtModemRX_t *dec)
{
    modem_decoder_t modem;
    modem.framesync = flexframesync_create(dec_framesync_callback, dec);
    flexframesync_set_header_len(modem.framesync, MODEM_HEADER_LEN);
    flexframesync_decode_header_soft(modem.framesync, 1);
    flexframesync_decode_payload_soft(modem.framesync, 1);
    dec->frame.modem = modem;
}

static void dec_gmsk_create(
                grtModemRX_t *dec)
{
    gmsk_decoder_t gmsk;
    gmsk.framesync = gmskframesync_create(dec_framesync_callback, dec);
    gmskframesync_set_header_len(gmsk.framesync, MODEM_HEADER_LEN);
    dec->frame.gmsk = gmsk;
}

grtModemRX_t *grtModemRX_create(
                const grtModemOpt_t *opt,
                size_t internal_bufsize)
{
    grtModemRX_t *dec = malloc(sizeof(grtModemRX_t));
    dec->opt = *opt;
    dec->frametype = opt->frametype; 
    flexframegenprops_init_default(&dec->fgprops);
    
    dec->fgprops.check = opt->frameopt->checksum_scheme;
    dec->fgprops.fec0 = opt->frameopt->inner_fec_scheme;    
    dec->fgprops.fec1 = opt->frameopt->outer_fec_scheme;
    dec->fgprops.mod_scheme = opt->frameopt->mod_scheme;
    switch (dec->frametype) {
        case frametype_ofdm:
            break;
        case frametype_modem:
            dec_modem_create(dec);
            break;
        case frametype_gmsk:
            dec_gmsk_create(dec);
            break;
        case frametype_unset:
            break;
    }
    dec->framelen = opt->frameopt->frame_len;
    dec->internal_bufsize = internal_bufsize;

    dec->dem = grtModulatorRX_create(
                              opt->modopt->shape, 
                              opt->modopt->samples_per_symbol,
                              opt->modopt->symbol_delay,
                              opt->modopt->excess_bw,
                              opt->modopt->center_rads,
                              opt->modopt->rxflt_order,
                              opt->modopt->rxflt_cutoff_frq,
                              opt->modopt->rxflt_center_frq,
                              1.0f,
                              60.0f);

    // FIXME buffer size
    size_t symbolbuf_len = dec->framelen*4;
    dec->symbolbuf = malloc(symbolbuf_len*sizeof(float complex));
    dec->symbolbuf_len = symbolbuf_len;
   
    dec->consume_cb = cbufferf_create(dec->internal_bufsize);
    grtModemRX_reset(dec); 
    dec->emit_callback = NULL;
    dec->emit_callback_userdata = NULL;
    dec->emit_dubug_callback = NULL;
    return dec;
}

void grtModemRX_destroy(
                grtModemRX_t *dec)
{
    if (!dec)
        return;
    switch (dec->frametype) {
        case frametype_ofdm:
            break;
        case frametype_modem:
            flexframesync_destroy(dec->frame.modem.framesync);
            break;
        case frametype_gmsk:
            gmskframesync_destroy(dec->frame.gmsk.framesync);
            break;
        case frametype_unset:
            break;
    }
    grtModulatorRX_destroy(dec->dem); 
    cbufferf_destroy(dec->consume_cb);
    free(dec->symbolbuf);
    free(dec);
}

void grtModemRX_flush(
                grtModemRX_t *dec)
{
    dec->flush = true;
}

void grtModemRX_reset(
                grtModemRX_t *dec)
{
    dec->flush = false;
    cbufferf_reset(dec->consume_cb);
}

size_t grtModemRX_consume(
                grtModemRX_t *dec,
                float *buffer, 
                size_t buflen)
{
    size_t avail = cbufferf_max_size(dec->consume_cb) - 
                       cbufferf_size(dec->consume_cb);
    if (avail < buflen) {
        return 0;
    }
    cbufferf_write(dec->consume_cb, buffer, buflen); 

    size_t stride = dec->framelen * dec->dem->samples_per_symbol;
    float *samples;
    unsigned int nread;

    while(true) {
        size_t count = cbufferf_size(dec->consume_cb);
        if (count < stride) {
            if (!dec->flush) {
                break;
            }
        }
        cbufferf_read(dec->consume_cb, stride, &samples, &nread);
        cbufferf_release(dec->consume_cb, nread);

        size_t symbols = grtModulatorRX_recv(dec->dem,
                                      samples,
                                      nread,
                                      dec->symbolbuf);
        if (symbols == 0)
            break;

        switch(dec->frametype) {
            case frametype_ofdm:
                break;
            case frametype_modem:
                flexframesync_execute(dec->frame.modem.framesync,
                                      dec->symbolbuf,
                                      symbols);
                break;
            case frametype_gmsk:
                gmskframesync_execute(dec->frame.gmsk.framesync,
                                      dec->symbolbuf,
                                      symbols);
                break;
            case frametype_unset:
                break;
        }
    }

    return buflen;
}

