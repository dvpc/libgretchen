#include "gretchen.internal.h"

static size_t framegen_estimate_num_symbols(
                grtModemTX_t *enc,
                size_t len)
{
    uint8_t *empty = calloc(len, sizeof(uint8_t));
    uint8_t header[MODEM_HEADER_LEN];
    memset(header, '\0', MODEM_HEADER_LEN);
    size_t num_symbols = 0;
    switch(enc->frametype) {
        case frametype_ofdm:
            break; 
        case frametype_modem:
            flexframegen_assemble(enc->frame.modem.framegen, 
                                  header, 
                                  empty, 
                                  len);
            num_symbols = flexframegen_getframelen(enc->frame.modem.framegen);
            flexframegen_reset(enc->frame.modem.framegen);
            break;
        case frametype_gmsk:
            gmskframegen_assemble(enc->frame.gmsk.framegen, 
                                  header, 
                                  empty, 
                                  len,
                                  enc->fgprops.check,
                                  enc->fgprops.fec0,
                                  enc->fgprops.fec1);
            num_symbols = gmskframegen_getframelen(enc->frame.gmsk.framegen);
            gmskframegen_reset(enc->frame.gmsk.framegen);
            break;
        case frametype_unset:
            break;
    }
    free(empty);
    return num_symbols;
}

static int framegen_is_assembled(
                grtModemTX_t *enc)
{
    switch(enc->frametype) {
        case frametype_ofdm:
            return 0;
        case frametype_modem:
            return flexframegen_is_assembled(enc->frame.modem.framegen);
        case frametype_gmsk:
            return gmskframegen_is_assembled(enc->frame.gmsk.framegen);
        case frametype_unset:
            return 0;
    }
}

static void framegen_assemble(
                grtModemTX_t *enc,
                uint8_t *readframe,
                size_t len)
{
    uint8_t header[MODEM_HEADER_LEN];
    memset(header, '\0', MODEM_HEADER_LEN);
    snprintf((char*)header, 
                    MODEM_HEADER_LEN, 
                    MODEM_HEADER_FORMAT, 
                    enc->hash, 
                    enc->frame_num,
                    enc->frame_nummax); 
    switch(enc->frametype) {
        case frametype_ofdm:
            break; 
        case frametype_modem:
            flexframegen_assemble(enc->frame.modem.framegen, 
                                  header, 
                                  readframe,
                                  len);
            enc->frame.modem.symbols_remaining = 
                    flexframegen_getframelen(enc->frame.modem.framegen);
            enc->frame_num ++;
            break;
        case frametype_gmsk:
            gmskframegen_reset(enc->frame.gmsk.framegen);
            gmskframegen_assemble(enc->frame.gmsk.framegen, 
                                  header, 
                                  readframe,
                                  len, 
                                  enc->fgprops.check,
                                  enc->fgprops.fec0,
                                  enc->fgprops.fec1);
            enc->frame_num ++;
            break;
        case frametype_unset:
            break;
    }
}

static size_t framegen_write_symbols(
                grtModemTX_t *enc,
                float complex *symbols,
                size_t len)
{
    size_t written = 0;
    int gmsk_rem = 0;
    switch(enc->frametype) {
        case frametype_ofdm:
            return written;
        case frametype_modem:
            if (len > enc->frame.modem.symbols_remaining)
                written = enc->frame.modem.symbols_remaining;
            else
                written = len;
            flexframegen_write_samples(enc->frame.modem.framegen,
                                       symbols,
                                       written);
            enc->frame.modem.symbols_remaining -= written;
            return written;
        case frametype_gmsk:
            gmsk_rem = len % enc->frame.gmsk.stride;
            if (gmsk_rem != 0)
                written = len + enc->frame.gmsk.stride - gmsk_rem;
            else
                written = len;
            size_t idx;
            for (idx=0; idx < written; idx+=enc->frame.gmsk.stride) {
                int done = gmskframegen_write_samples(enc->frame.gmsk.framegen,
                                                      symbols + idx);
                if (done)
                    break; 
            }
            return idx;
        case frametype_unset:
            return written;
    }
}

static void process_frames(
                grtModemTX_t *enc)
{
    size_t cons_want = enc->framelen;
    size_t frames_want = enc->framelen_symbols;
    while(true) {
        // 0 check if consume rbuf has enough data to begin processing
        size_t cons_count = enc->cons_rbuf->count;
        if (cons_count < cons_want) {
            if (enc->flush) 
                cons_want = cons_count;
            else
                break; 
        }
        // 1 assemble frame
        if (!framegen_is_assembled(enc)) {
            enc->cons_rbuf_err = rbufuPop(enc->cons_rbuf, 
                                          enc->buf_readframe, 
                                          cons_want);
            if (enc->cons_rbuf_err!=rbufNoError) {
                break;
            }
            framegen_assemble(enc, enc->buf_readframe, cons_want); 
            continue;
        }
        // 2 write the frame
        float complex symbols[frames_want];
        size_t symwrit = framegen_write_symbols(enc,
                                                symbols, 
                                                frames_want);
        // 3 modulate symbols
        size_t smpwrit = grtModulatorTX_recv(enc->mod, 
                                      symbols, 
                                      symwrit, 
                                      enc->buf_samples);
        size_t flushwrit = grtModulatorTX_flush(enc->mod, 
                                         enc->buf_flush);
        // 4 call callback method with data
        if (enc->emit_callback) {
            enc->emit_callback(smpwrit, enc->buf_samples, enc->emit_callback_userdata);
            enc->emit_callback(flushwrit, enc->buf_flush, enc->emit_callback_userdata);
        }
    }
}

static void enc_modem_create(
                grtModemTX_t *enc)
{
    modem_encoder_t modem;
    modem.framegen = flexframegen_create(&enc->fgprops);
    flexframegen_set_header_len(modem.framegen, MODEM_HEADER_LEN);
    modem.symbols_remaining = 0;
    enc->frame.modem = modem;
}

static void enc_gmsk_create(
                grtModemTX_t *enc) 
{
    gmsk_encoder_t gmsk;
    gmsk.framegen = gmskframegen_create();
    gmskframegen_set_header_len(gmsk.framegen, MODEM_HEADER_LEN);
    gmsk.stride = 2;
    enc->frame.gmsk = gmsk;
}

grtModemTX_t *grtModemTX_create(
                const grtModemOpt_t *opt,
                size_t internal_bufsize)
{
    grtModemTX_t *enc = malloc(sizeof(grtModemTX_t));
    enc->opt = *opt;
    enc->frametype = opt->frametype;
    flexframegenprops_init_default(&enc->fgprops);
    enc->fgprops.check = opt->frameopt->checksum_scheme;
    enc->fgprops.fec0 = opt->frameopt->inner_fec_scheme;    
    enc->fgprops.fec1 = opt->frameopt->outer_fec_scheme;
    enc->fgprops.mod_scheme = opt->frameopt->mod_scheme;
    switch (enc->frametype) {
        case frametype_ofdm:
            break; 
        case frametype_modem:
            enc_modem_create(enc);
            break;
        case frametype_gmsk:
            enc_gmsk_create(enc);
            break;
        case frametype_unset:
            break;
    }
    enc->framelen = opt->frameopt->frame_len;
    enc->framelen_symbols = framegen_estimate_num_symbols(enc, opt->frameopt->frame_len);
    enc->internal_bufsize = internal_bufsize;

    enc->mod = grtModulatorTX_create(opt->modopt->shape,
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

    enc->buf_readframe = calloc(enc->framelen,
                                sizeof(uint8_t));
    enc->buf_samples = calloc(enc->mod->samples_per_symbol*enc->framelen_symbols,
                                sizeof(float));
    enc->buf_flush = calloc(enc->mod->samples_per_symbol*enc->mod->flushlen,
                                sizeof(float));

    enc->cons_rbuf = rbufuCreate(enc->internal_bufsize);
    grtModemTX_reset(enc);
    enc->emit_callback = NULL;
    enc->emit_callback_userdata = NULL;
    return enc;
}

void grtModemTX_destroy(
                grtModemTX_t *enc)
{
    if (!enc)
        return;
    switch (enc->frametype) {
        case frametype_ofdm:
            break; 
        case frametype_modem:
            flexframegen_destroy(enc->frame.modem.framegen);
            break;
        case frametype_gmsk:
            gmskframegen_destroy(enc->frame.gmsk.framegen);
            break;
        case frametype_unset:
            break;
    }
    rbufuDestroy(enc->cons_rbuf);
    free(enc->buf_readframe);
    free(enc->buf_samples);
    free(enc->buf_flush);
    grtModulatorTX_destroy(enc->mod);
    free(enc);
}

void grtModemTX_setheaderinfo(
                grtModemTX_t *enc,
                unsigned long filehash, 
                size_t filesize) 
{
    enc->hash = filehash;
    enc->frame_nummax = filesize/enc->framelen+1;
} 

void grtModemTX_enable_flush(
                grtModemTX_t *enc)
{
    enc->flush = true;
}

void grtModemTX_reset(
                grtModemTX_t *enc)
{
    enc->flush = false;
    enc->hash = 0;
    enc->frame_num = 0;
    enc->frame_nummax = 0;
    grtModulatorTX_reset(enc->mod);
    rbufuReset(enc->cons_rbuf);
}

size_t grtModemTX_consume(
                grtModemTX_t *enc,
                const void *buffer, 
                size_t buflen)
{
    size_t avail = rbufuAvailable(enc->cons_rbuf);
    if (avail==0)
        return 0;
    if (avail<buflen)
        buflen = avail;
    enc->cons_rbuf_err = rbufuPush(enc->cons_rbuf, 
                                   buffer, 
                                   buflen);
    if (enc->cons_rbuf_err != rbufNoError) {
        return 0;
    }

    process_frames(enc); 

    return buflen;
}



