/*
 * Gretchen 
 * Modem and Modulator
 */
#ifndef ___GRT_INTERNAL___
#define ___GRT_INTERNAL___

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <liquid/liquid.h>
#include "internal/ringbuffer_u.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MODEM OPTIONS 

typedef enum frametypes {
    frametype_unset,
    frametype_ofdm,
    frametype_modem,
    frametype_gmsk
} grtFrameType_t;

typedef struct {
    size_t frame_len;
    crc_scheme checksum_scheme;
    fec_scheme inner_fec_scheme;
    fec_scheme outer_fec_scheme;
    modulation_scheme mod_scheme;
    unsigned int _bits_per_symbol;
} grtFrameOpt_t;

typedef struct {
    unsigned int shape;  
    unsigned int samples_per_symbol;
    unsigned int symbol_delay;
    float excess_bw;
    float center_rads;
    float gain;
    unsigned int flushlen_mod;
    unsigned int txflt_order;
    float txflt_cutoff_frq;
    float txflt_center_frq;
    unsigned int rxflt_order;
    float rxflt_cutoff_frq;
    float rxflt_center_frq;
} grtModulatorOpt_t;

typedef struct {
    grtFrameType_t frametype;
    grtFrameOpt_t *frameopt;
    grtModulatorOpt_t *modopt;
} grtModemOpt_t;

grtModemOpt_t *grtModemOpt_parse_args(int argc, char** argv, bool is_enc);

void grtModemOpt_destroy(grtModemOpt_t* opt);

void grtModemOpt_print(grtModemOpt_t* opt);

grtModemOpt_t* grtModemOpt_create_empty();

float grtModemOpt_convert_freq2rad(int frequency, int samplerate); 

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Modem 

#define MODEM_HEADER_LEN 2+sizeof(unsigned long)*3 
#define MODEM_HEADER_FORMAT "%lu,%d,%d"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ModulatorRX (Demodulator)
 
typedef struct {
    nco_crcf nco;
    firdecim_crcf decim;
    iirfilt_rrrf filter_rx;
    unsigned int samples_per_symbol;
} grtModulatorRX_t;

grtModulatorRX_t *grtModulatorRX_create(unsigned int shape, unsigned int samples_per_symbol, unsigned int symbol_delay, float excess_bw, float center_rads, unsigned int flt_order, float flt_cutoff_frq, float flt_center_frq, float flt_passband_ripple, float flt_stopband_ripple);

void grtModulatorRX_destroy(grtModulatorRX_t *dem);

size_t grtModulatorRX_recv(grtModulatorRX_t *dem, float *samples, size_t samples_len, float complex *symbols);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ModemRX (Decoder)

typedef void grtModemRX_emit_callback(unsigned long hash, unsigned int frame_num, unsigned int frame_nummax, size_t buffer_len, uint8_t *buffer, void *user);

typedef void grtModemRX_emit_debug_callback(int header_valid, int payload_valid, unsigned int payload_len, framesyncstats_s stats);

typedef struct { 
    flexframesync framesync; 
} modem_decoder_t;

typedef struct { 
    gmskframesync framesync; 
} gmsk_decoder_t;

typedef struct {
    bool flush;
    grtFrameType_t frametype;

    size_t framelen;
    flexframegenprops_s fgprops;
    size_t internal_bufsize;

    grtModemOpt_t opt;
    union {
        modem_decoder_t modem;
        gmsk_decoder_t gmsk;
    } frame;
    grtModulatorRX_t *dem;

    float complex* symbolbuf;
    size_t symbolbuf_len;

    cbufferf consume_cb;

    grtModemRX_emit_callback *emit_callback;
    grtModemRX_emit_debug_callback *emit_dubug_callback;
    void *emit_callback_userdata;
} grtModemRX_t;

grtModemRX_t *grtModemRX_create(const grtModemOpt_t *opt, size_t internal_bufsize);

void grtModemRX_destroy(grtModemRX_t *dec);

void grtModemRX_flush(grtModemRX_t *dec);

void grtModemRX_reset(grtModemRX_t *dec);

size_t grtModemRX_consume(grtModemRX_t *dec, float *buffer, size_t buflen);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ModulatorTX 

typedef struct {
    nco_crcf nco;
    firinterp_crcf interp;
    iirfilt_crcf filter_tx;
    unsigned int samples_per_symbol;
    float gain;
    size_t flushlen;
} grtModulatorTX_t;

grtModulatorTX_t *grtModulatorTX_create(unsigned int shape, unsigned int samples_per_symbol, unsigned int symbol_delay, float excess_bw, float center_rads, float gain, unsigned int flt_order, float flt_cutoff_frq, float flt_center_frq, float flt_passband_ripple, float flt_stopband_ripple, unsigned int flushlen_mod);

void grtModulatorTX_destroy(grtModulatorTX_t *mod);

size_t grtModulatorTX_recv(grtModulatorTX_t *mod, float complex *symbols, size_t symbols_len, float *samples);

size_t grtModulatorTX_flush(grtModulatorTX_t *mod, float *samples);

void grtModulatorTX_reset(grtModulatorTX_t *mod);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ModemTX (Encoder) 

typedef void grtModemTX_emit_callback(size_t buffer_len, float *buffer, void *user);

typedef struct {
    flexframegen framegen;
    size_t symbols_remaining;
} modem_encoder_t;

typedef struct {
    gmskframegen framegen;
    size_t stride;
} gmsk_encoder_t;

typedef struct {
    bool flush;
    grtModemOpt_t opt;
    grtFrameType_t frametype;
    size_t framelen;
    size_t framelen_symbols;
    flexframegenprops_s fgprops;
    size_t internal_bufsize;
    union {
        modem_encoder_t modem;
        gmsk_encoder_t gmsk;
    } frame;
    grtModulatorTX_t *mod;

    rbufu_t* cons_rbuf;
    rbufError cons_rbuf_err;

    uint8_t *buf_readframe;
    float *buf_samples;
    float *buf_flush;
    
    grtModemTX_emit_callback *emit_callback;
    void *emit_callback_userdata;

    unsigned long hash;
    unsigned int frame_num;
    unsigned int frame_nummax;
} grtModemTX_t;

grtModemTX_t *grtModemTX_create(const grtModemOpt_t *opt, size_t internal_bufsize);

void grtModemTX_destroy(grtModemTX_t *enc);

void grtModemTX_setheaderinfo(grtModemTX_t *enc, unsigned long filehash, size_t filesize); 

void grtModemTX_enable_flush(grtModemTX_t *enc);

void grtModemTX_reset(grtModemTX_t *enc);

size_t grtModemTX_consume(grtModemTX_t *enc, const void *buffer, size_t buflen);

#endif
