#include "gretchen.internal.h"

static uint8_t framegen_is_assembled();
static void framegen_assemble();
static size_t framegen_write_symbols();
static void process_frames();
static void mtx_modem_create();
static void mtx_gmsk_create();
static void mtx_ofdm_create();

grtModemTX_t *grtModemTX_create(
                const grtModemOpt_t *opt,
                size_t internal_bufsize)
{
    grtModemTX_t *mtx = malloc(sizeof(grtModemTX_t));
    mtx->opt = *opt;
    mtx->frametype = opt->frametype;
    flexframegenprops_init_default(&mtx->fgprops);
    mtx->fgprops.check = opt->frameopt->checksum_scheme;
    mtx->fgprops.fec0 = opt->frameopt->inner_fec_scheme;    
    mtx->fgprops.fec1 = opt->frameopt->outer_fec_scheme;
    mtx->fgprops.mod_scheme = opt->frameopt->mod_scheme;
    switch (mtx->frametype) {
        case frametype_ofdm:
            mtx_ofdm_create(mtx);
            break; 
        case frametype_modem:
            mtx_modem_create(mtx);
            break;
        case frametype_gmsk:
            mtx_gmsk_create(mtx);
            break;
        case frametype_unset:
            break;
    }
    mtx->framelen = opt->frameopt->payload_len;
    mtx->framelen_symbols = framegen_estimate_num_symbols(mtx, opt->frameopt->payload_len);
    mtx->internal_bufsize = internal_bufsize;

    mtx->mod = grtModulatorTX_create(opt->modopt->shape,
                              opt->modopt->samples_per_symbol, 
                              opt->modopt->symbol_delay, 
                              opt->modopt->excess_bw, 
                              opt->modopt->center_rads, 
                              opt->modopt->gain,
                              opt->modopt->txflt_order,
                              opt->modopt->txflt_cutoff_frq,
                              opt->modopt->txflt_center_frq,
                              1.0f,
                              60.0f,
                              opt->modopt->flushlen_mod);

    mtx->buf_readframe = calloc(mtx->framelen,
                                sizeof(uint8_t));
    mtx->buf_samples = calloc(mtx->mod->samples_per_symbol*mtx->framelen_symbols,
                                sizeof(float));
    mtx->buf_flush = calloc(mtx->mod->samples_per_symbol*mtx->mod->flushlen,
                                sizeof(float));

    mtx->cons_rbuf = rbufuCreate(mtx->internal_bufsize);
    grtModemTX_reset(mtx);
    mtx->emit_callback = NULL;
    mtx->emit_callback_userdata = NULL;
    return mtx;
}

void grtModemTX_destroy(grtModemTX_t *mtx)
{
    if (!mtx)
        return;
    switch (mtx->frametype) {
        case frametype_ofdm:
            ofdmflexframegen_destroy(mtx->frame.ofdm.framegen);
            break; 
        case frametype_modem:
            flexframegen_destroy(mtx->frame.modem.framegen);
            break;
        case frametype_gmsk:
            gmskframegen_destroy(mtx->frame.gmsk.framegen);
            break;
        case frametype_unset:
            break;
    }
    rbufuDestroy(mtx->cons_rbuf);
    free(mtx->buf_readframe);
    free(mtx->buf_samples);
    free(mtx->buf_flush);
    grtModulatorTX_destroy(mtx->mod);
    free(mtx);
}

void grtModemTX_setheaderinfo(
                grtModemTX_t *mtx,
                uint64_t filehash, 
                size_t filesize) 
{
    mtx->hash = filehash;
    mtx->frame_nummax = filesize/mtx->framelen+1;
} 

void grtModemTX_enable_flush(grtModemTX_t *mtx)
{
    mtx->flush = true;
}

void grtModemTX_reset(grtModemTX_t *mtx)
{
    mtx->flush = false;
    mtx->hash = 0;
    mtx->frame_num = 0;
    mtx->frame_nummax = 0;
    grtModulatorTX_reset(mtx->mod);
    rbufuReset(mtx->cons_rbuf);
}

size_t grtModemTX_consume(
                grtModemTX_t *mtx,
                const void *buffer, 
                size_t buflen)
{
    size_t avail = rbufuAvailable(mtx->cons_rbuf);
    if (avail==0)
        return 0;
    if (avail<buflen)
        buflen = avail;
    int8_t err = rbufuPush(mtx->cons_rbuf, 
                                   buffer, 
                                   buflen);
    if (err!=RBUF_OK) {
        return 0;
    }

    process_frames(mtx); 

    return buflen;
}

size_t framegen_estimate_num_symbols(
                grtModemTX_t *mtx,
                size_t len)
{
    uint8_t *empty = calloc(len, sizeof(uint8_t));
    uint8_t header[MODEM_HEADER_LEN];
    memset(header, '\0', MODEM_HEADER_LEN);
    size_t num_symbols = 0;
    switch(mtx->frametype) {
        case frametype_ofdm:
            ofdmflexframegen_assemble(mtx->frame.ofdm.framegen, 
                                      header,
                                      empty,
                                      len);
            size_t blocks = ofdmflexframegen_getframelen(mtx->frame.ofdm.framegen);
            num_symbols = blocks * mtx->opt.ofdmopt->num_subcarriers + 
                    mtx->opt.ofdmopt->cyclic_prefix_len; 
            ofdmflexframegen_reset(mtx->frame.ofdm.framegen);
            break; 
        case frametype_modem:
            flexframegen_assemble(mtx->frame.modem.framegen, 
                                  header, 
                                  empty, 
                                  len);
            num_symbols = flexframegen_getframelen(mtx->frame.modem.framegen);
            flexframegen_reset(mtx->frame.modem.framegen);
            break;
        case frametype_gmsk:
            gmskframegen_assemble(mtx->frame.gmsk.framegen, 
                                  header, 
                                  empty, 
                                  len,
                                  mtx->fgprops.check,
                                  mtx->fgprops.fec0,
                                  mtx->fgprops.fec1);
            num_symbols = gmskframegen_getframelen(mtx->frame.gmsk.framegen);
            gmskframegen_reset(mtx->frame.gmsk.framegen);
            break;
        case frametype_unset:
            break;
    }
    free(empty);
    return num_symbols;
}

static uint8_t framegen_is_assembled(grtModemTX_t *mtx)
{
    switch(mtx->frametype) {
        case frametype_ofdm:
            return ofdmflexframegen_is_assembled(mtx->frame.ofdm.framegen);
        case frametype_modem:
            return flexframegen_is_assembled(mtx->frame.modem.framegen);
        case frametype_gmsk:
            return gmskframegen_is_assembled(mtx->frame.gmsk.framegen);
        case frametype_unset:
            return 0;
    }
}

static void framegen_assemble(
                grtModemTX_t *mtx,
                uint8_t *readframe,
                size_t len)
{
    uint8_t header[MODEM_HEADER_LEN];
    memset(header, '\0', MODEM_HEADER_LEN);
    snprintf((uint8_t*)header, 
                    MODEM_HEADER_LEN, 
                    MODEM_HEADER_FORMAT, 
                    mtx->hash, 
                    mtx->frame_num,
                    mtx->frame_nummax); 
    switch(mtx->frametype) {
        case frametype_ofdm:
            ofdmflexframegen_assemble(mtx->frame.ofdm.framegen,
                                      header,
                                      readframe,
                                      len);
            mtx->frame_num ++;
            break; 
        case frametype_modem:
            flexframegen_assemble(mtx->frame.modem.framegen, 
                                  header, 
                                  readframe,
                                  len);
            mtx->frame.modem.symbols_remaining = 
                    flexframegen_getframelen(mtx->frame.modem.framegen);
            mtx->frame_num ++;
            break;
        case frametype_gmsk:
            gmskframegen_reset(mtx->frame.gmsk.framegen);
            gmskframegen_assemble(mtx->frame.gmsk.framegen, 
                                  header, 
                                  readframe,
                                  len, 
                                  mtx->fgprops.check,
                                  mtx->fgprops.fec0,
                                  mtx->fgprops.fec1);
            mtx->frame_num ++;
            break;
        case frametype_unset:
            break;
    }
}

static size_t framegen_write_symbols(
                grtModemTX_t *mtx,
                float complex *symbols,
                size_t len)
{
    size_t written = 0;
    uint32_t gmsk_rem = 0;
    switch(mtx->frametype) {
        case frametype_ofdm:
            ofdmflexframegen_write(
                                mtx->frame.ofdm.framegen,
                                symbols,
                                len);
            return len;
        case frametype_modem:
            if (len > mtx->frame.modem.symbols_remaining)
                written = mtx->frame.modem.symbols_remaining;
            else
                written = len;
            flexframegen_write_samples(
                            mtx->frame.modem.framegen,
                            symbols,
                            written);
            mtx->frame.modem.symbols_remaining -= written;
            return written;
        case frametype_gmsk:
            gmsk_rem = len % mtx->frame.gmsk.stride;
            if (gmsk_rem != 0)
                written = len + mtx->frame.gmsk.stride - gmsk_rem;
            else
                written = len;
            size_t idx;
            for (idx=0; idx < written; idx+=mtx->frame.gmsk.stride) {
                uint8_t done = gmskframegen_write_samples(
                                mtx->frame.gmsk.framegen,
                                symbols + idx);
                if (done)
                    break; 
            }
            return idx;
        case frametype_unset:
            return written;
    }
}

static void process_frames(grtModemTX_t *mtx)
{
    size_t cons_want = mtx->framelen;
    size_t frames_want = mtx->framelen_symbols;
    while(true) {
        // 0 check if consume rbuf has enough data to begin processing
        size_t cons_count = mtx->cons_rbuf->count;
        if (cons_count < cons_want) {
            if (mtx->flush) 
                cons_want = cons_count;
            else
                break; 
        }
        // 1 assemble frame
        if (!framegen_is_assembled(mtx)) {
            int8_t err = rbufuPop(mtx->cons_rbuf, 
                                          mtx->buf_readframe, 
                                          cons_want);
            if (err!=RBUF_OK) {
                break;
            }
            framegen_assemble(mtx, mtx->buf_readframe, cons_want); 
            continue;
        }
        // 2 write the frame
        float complex symbols[frames_want];
        size_t symwrit = framegen_write_symbols(mtx, symbols, frames_want);
        // 3 modulate symbols
        size_t smpwrit = grtModulatorTX_recv(mtx->mod, 
                                             symbols, 
                                             symwrit, 
                                             mtx->buf_samples);
        size_t flushwrit = grtModulatorTX_flush(mtx->mod, mtx->buf_flush);
        // 4 call callback method with data
        if (mtx->emit_callback) {
            mtx->emit_callback(smpwrit, mtx->buf_samples, mtx->emit_callback_userdata);
            if (mtx->frametype!=frametype_ofdm)
                mtx->emit_callback(flushwrit, mtx->buf_flush, mtx->emit_callback_userdata);
        }
    }
}

static void mtx_modem_create(grtModemTX_t *mtx)
{
    modem_encoder_t modem;
    modem.framegen = flexframegen_create(&mtx->fgprops);
    flexframegen_set_header_len(modem.framegen, MODEM_HEADER_LEN);
    modem.symbols_remaining = 0;
    mtx->frame.modem = modem;
}

static void mtx_gmsk_create(grtModemTX_t *mtx) 
{
    gmsk_encoder_t gmsk;
    gmsk.framegen = gmskframegen_create();
    gmskframegen_set_header_len(gmsk.framegen, MODEM_HEADER_LEN);
    gmsk.stride = 2;
    mtx->frame.gmsk = gmsk;
}

static void mtx_ofdm_create(grtModemTX_t *mtx)
{
    ofdmflexframegenprops_s fgprops;
    ofdmflexframegenprops_init_default(&fgprops);
    fgprops.check           = mtx->fgprops.check;
    fgprops.fec0            = mtx->fgprops.fec0;
    fgprops.fec1            = mtx->fgprops.fec1;
    fgprops.mod_scheme      = mtx->fgprops.mod_scheme;
    ofdm_encoder_t ofdm;
    ofdm.framegen = ofdmflexframegen_create(
                    mtx->opt.ofdmopt->num_subcarriers,
                    mtx->opt.ofdmopt->cyclic_prefix_len,
                    mtx->opt.ofdmopt->taper_len,
                    NULL,
                    &fgprops); 
    ofdmflexframegen_set_header_len(ofdm.framegen, MODEM_HEADER_LEN);
    mtx->frame.ofdm = ofdm;
}
