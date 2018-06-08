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
    grtModemRX_t *mrx = dvoid;
    if (mrx->emit_debug_callback)
        mrx->emit_debug_callback(header_valid,
                                 payload_valid, 
                                 payload_len,
                                 _stats);
    if (!header_valid)
        return 1;
    if (!payload_valid)
        return 1;
    if (mrx->emit_callback) {
        unsigned long hash; 
        unsigned int frame_num, frame_nummax;
        sscanf((char *)header,
               MODEM_HEADER_FORMAT, 
               &hash, 
               &frame_num,
               &frame_nummax);
        mrx->emit_callback(hash,
                           frame_num,
                           frame_nummax,
                           payload_len, 
                           payload, 
                           mrx->emit_callback_userdata);
    }
    return 0;
}

static void dec_modem_create(
                grtModemRX_t *mrx)
{
    modem_decoder_t modem;
    modem.framesync = flexframesync_create(dec_framesync_callback, mrx);
    flexframesync_set_header_len(modem.framesync, MODEM_HEADER_LEN);
    flexframesync_decode_header_soft(modem.framesync, 1);
    flexframesync_decode_payload_soft(modem.framesync, 1);
    mrx->frame.modem = modem;
}

static void dec_gmsk_create(
                grtModemRX_t *mrx)
{
    gmsk_decoder_t gmsk;
    gmsk.framesync = gmskframesync_create(dec_framesync_callback, mrx);
    gmskframesync_set_header_len(gmsk.framesync, MODEM_HEADER_LEN);
    mrx->frame.gmsk = gmsk;
}

grtModemRX_t *grtModemRX_create(
                const grtModemOpt_t *opt,
                size_t internal_bufsize)
{
    grtModemRX_t *mrx = malloc(sizeof(grtModemRX_t));
    mrx->opt = *opt;
    mrx->frametype = opt->frametype; 
    flexframegenprops_init_default(&mrx->fgprops);
    
    mrx->fgprops.check = opt->frameopt->checksum_scheme;
    mrx->fgprops.fec0 = opt->frameopt->inner_fec_scheme;    
    mrx->fgprops.fec1 = opt->frameopt->outer_fec_scheme;
    mrx->fgprops.mod_scheme = opt->frameopt->mod_scheme;
    switch (mrx->frametype) {
        case frametype_ofdm:
            break;
        case frametype_modem:
            dec_modem_create(mrx);
            break;
        case frametype_gmsk:
            dec_gmsk_create(mrx);
            break;
        case frametype_unset:
            break;
    }
    mrx->framelen = opt->frameopt->payload_len;
    mrx->internal_bufsize = internal_bufsize;

    mrx->dem = grtModulatorRX_create(
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
    size_t symbolbuf_len = mrx->framelen*4;
    mrx->symbolbuf = malloc(symbolbuf_len*sizeof(float complex));
    mrx->symbolbuf_len = symbolbuf_len;
   
    mrx->consume_cb = cbufferf_create(mrx->internal_bufsize);
    grtModemRX_reset(mrx); 
    mrx->emit_callback = NULL;
    mrx->emit_callback_userdata = NULL;
    mrx->emit_debug_callback = NULL;
    return mrx;
}

void grtModemRX_destroy(
                grtModemRX_t *mrx)
{
    if (!mrx)
        return;
    switch (mrx->frametype) {
        case frametype_ofdm:
            break;
        case frametype_modem:
            flexframesync_destroy(mrx->frame.modem.framesync);
            break;
        case frametype_gmsk:
            gmskframesync_destroy(mrx->frame.gmsk.framesync);
            break;
        case frametype_unset:
            break;
    }
    grtModulatorRX_destroy(mrx->dem); 
    cbufferf_destroy(mrx->consume_cb);
    free(mrx->symbolbuf);
    free(mrx);
}

void grtModemRX_flush(
                grtModemRX_t *mrx)
{
    mrx->flush = true;
}

void grtModemRX_reset(
                grtModemRX_t *mrx)
{
    mrx->flush = false;
    cbufferf_reset(mrx->consume_cb);
}

size_t grtModemRX_consume(
                grtModemRX_t *mrx,
                float *buffer, 
                size_t buflen)
{
    size_t avail = cbufferf_max_size(mrx->consume_cb) - 
                       cbufferf_size(mrx->consume_cb);
    if (avail < buflen) {
        return 0;
    }
    cbufferf_write(mrx->consume_cb, buffer, buflen); 

    size_t stride = mrx->framelen * mrx->dem->samples_per_symbol;
    float *samples;
    unsigned int nread;

    while(true) {
        size_t count = cbufferf_size(mrx->consume_cb);
        if (count < stride) {
            if (!mrx->flush) {
                break;
            }
        }
        cbufferf_read(mrx->consume_cb, stride, &samples, &nread);
        cbufferf_release(mrx->consume_cb, nread);

        size_t symbols = grtModulatorRX_recv(mrx->dem,
                                      samples,
                                      nread,
                                      mrx->symbolbuf);
        if (symbols == 0)
            break;

        switch(mrx->frametype) {
            case frametype_ofdm:
                break;
            case frametype_modem:
                flexframesync_execute(mrx->frame.modem.framesync,
                                      mrx->symbolbuf,
                                      symbols);
                break;
            case frametype_gmsk:
                gmskframesync_execute(mrx->frame.gmsk.framesync,
                                      mrx->symbolbuf,
                                      symbols);
                break;
            case frametype_unset:
                break;
        }
    }

    return buflen;
}

