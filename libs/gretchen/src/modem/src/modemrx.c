#include "gretchen.internal.h"

static int mrx_framesync_callback();
static void mrx_modem_create();
static void mrx_gmsk_create();
static void mrx_ofdm_create();

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
            mrx_ofdm_create(mrx);
            break;
        case frametype_modem:
            mrx_modem_create(mrx);
            break;
        case frametype_gmsk:
            mrx_gmsk_create(mrx);
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

void grtModemRX_destroy(grtModemRX_t *mrx)
{
    if (!mrx)
        return;
    switch (mrx->frametype) {
        case frametype_ofdm:
            ofdmflexframesync_destroy(mrx->frame.ofdm.framesync);
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

void grtModemRX_enable_flush(grtModemRX_t *mrx)
{
    mrx->flush = true;
}

void grtModemRX_reset(grtModemRX_t *mrx)
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
    uint32_t nread;

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
                ofdmflexframesync_execute(mrx->frame.ofdm.framesync,
                                          mrx->symbolbuf,
                                          symbols);
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

static int mrx_framesync_callback(
                uint8_t *header, 
                int header_valid, 
                uint8_t *payload,
                uint32_t payload_len, 
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
        mrx->emit_debug_callback(
                        header_valid,
                        payload_valid, 
                        payload_len,
                        _stats);
    if (!header_valid)
        return 1;
    uint64_t hash; 
    uint32_t frame_num, frame_nummax;
    sscanf((char*)header,
           MODEM_HEADER_FORMAT, 
           &hash, 
           &frame_num,
           &frame_nummax);
    if (mrx->emit_callback && payload_valid)
        mrx->emit_callback(
                        hash,
                        frame_num,
                        frame_nummax,
                        payload_len, 
                        payload, 
                        mrx->emit_callback_userdata);
    if (mrx->emit_progress_callback)
        mrx->emit_progress_callback(
                        hash, 
                        frame_num, 
                        frame_nummax, 
                        payload_valid,
                        mrx->emit_callback_userdata);
    if (!payload_valid)
        return 1;
    return 0;
}

static void mrx_modem_create(grtModemRX_t *mrx)
{
    modem_decoder_t modem;
    modem.framesync = flexframesync_create(mrx_framesync_callback, mrx);
    flexframesync_set_header_len(modem.framesync, MODEM_HEADER_LEN);
    flexframesync_decode_header_soft(modem.framesync, 1);
    flexframesync_decode_payload_soft(modem.framesync, 1);
    mrx->frame.modem = modem;
}

static void mrx_gmsk_create(grtModemRX_t *mrx)
{
    gmsk_decoder_t gmsk;
    gmsk.framesync = gmskframesync_create(mrx_framesync_callback, mrx);
    gmskframesync_set_header_len(gmsk.framesync, MODEM_HEADER_LEN);
    mrx->frame.gmsk = gmsk;
}

static void mrx_ofdm_create(grtModemRX_t *mrx)
{
    ofdm_decoder_t ofdm;
    ofdm.framesync = ofdmflexframesync_create(
                    mrx->opt.ofdmopt->num_subcarriers,
                    mrx->opt.ofdmopt->cyclic_prefix_len,
                    mrx->opt.ofdmopt->taper_len,
                    NULL,
                    mrx_framesync_callback, 
                    mrx); 
    ofdmflexframesync_set_header_len(ofdm.framesync, MODEM_HEADER_LEN);
    ofdmflexframesync_decode_header_soft(ofdm.framesync, 1);
    ofdmflexframesync_decode_payload_soft(ofdm.framesync, 1);
    mrx->frame.ofdm = ofdm;
}



