/*
 * Gretchen internal
 */
#ifndef ___GRETCHEN_INTERNAL_H___
#define ___GRETCHEN_INTERNAL_H___

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ringbuffer
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/* 
 * Simple ring buffer with memcpy
 * not thread safe!
 * ugly and simple :D
 *
 * NOTE!!!!!!! about memcpy
 * Some notes on memcpy
 * http://brnz.org/hbr/?p=1094
 */
#ifndef ___RINGBUFFER_U___
#define ___RINGBUFFER_U___

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum rbufError {
    rbufNoError = 0,
    rbufErrorEmpty = -1,
    rbufErrorFull = -2,
    rbufErrorRequestTooLarge = -3,  
    rbufErrorBusy = -4
} rbufError;

typedef uint8_t RBUF_U_TYPE;

typedef struct {
    RBUF_U_TYPE *buffer;
    RBUF_U_TYPE *head;
    RBUF_U_TYPE *tail;
    size_t maxlen;
    size_t count;
    int haslockpop;
    int haslockpush;
} rbufu_t;

/*
 * Returns a pointer to created ringbuffer of size len. 
 * NULL on error.
 */
rbufu_t* rbufuCreate(size_t len);

/*
 * Destroys the ringbuffer.
 */
void rbufuDestroy(rbufu_t* cb);

/*
 * Returns the number of available elements to store.
 * AKA free space.
 */
size_t rbufuAvailable(const rbufu_t* cb);

/*
 * Passes a buffer of size len to be pushed into the ringbuffer.
 * Returns `rbufErrorRequestTooLarge` error if buffer is greater 
 * than the internal available space.
 * If the buffer is full it returns a `rbufErrorFull` error. 
 * If all went well a `rbufNoError` is returned.
 */
rbufError rbufuPush(rbufu_t* cb, const RBUF_U_TYPE* ibuf, size_t len);

/*
 * Fills a passed buffer of len with internal data. 
 * If more data is reuqested than available it returns 
 * a `rbufErrorRequestTooLarge` error. 
 * If the buffer is empty a `rbufErrorEmpty` error is returned.
 * If all went well a `rbufNoError` is returned.
 */
rbufError rbufuPop(rbufu_t* cb, RBUF_U_TYPE* obuf, size_t len);

/*
 * Prints the whole internal buffer state to stdout.
 * Idk, but maybe replace with fprintf to strerr?? 
 */
void rbufuPrintBuffer(const rbufu_t* cb);

/*
 * Resets the internal state of the ringbuffer
 */
void rbufuReset(rbufu_t* cb);

#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// hashmap
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*
 * Generic hashmap manipulation functions
 *
 * Originally by Elliot C Back - http://elliottback.com/wp/hashmap-implementation-in-c/
 *
 * Modified by Pete Warden to fix a serious performance problem, support strings as keys
 * and removed thread synchronization - http://petewarden.typepad.com
 */
#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#define MAP_MISSING -3  /* No such element */
#define MAP_FULL -2 	/* Hashmap is full */
#define MAP_OMEM -1 	/* Out of Memory */
#define MAP_OK 0 	/* OK */

/*
 * any_t is a pointer.  This allows you to put arbitrary structures in
 * the hashmap.
 */
typedef void *any_t;

/*
 * PFany is a pointer to a function that can take two any_t arguments
 * and return an integer. Returns status code..
 */
typedef int (*PFany)(any_t, any_t);

/*
 * map_t is a pointer to an internally maintained data structure.
 * Clients of this package do not need to know how hashmaps are
 * represented.  They see and manipulate only map_t's.
 */
typedef any_t map_t;

/*
 * Return an empty hashmap. Returns NULL if empty.
*/
extern map_t hashmap_new();

/*
 * Iteratively call f with argument (item, data) for
 * each element data in the hashmap. The function must
 * return a map status code. If it returns anything other
 * than MAP_OK the traversal is terminated. f must
 * not reenter any hashmap functions, or deadlock may arise.
 */
extern int hashmap_iterate(map_t in, PFany f, any_t item);

/*
 * Add an element to the hashmap. Return MAP_OK or MAP_OMEM.
 */
extern int hashmap_put(map_t in, char* key, any_t value);

/*
 * Get an element from the hashmap. Return MAP_OK or MAP_MISSING.
 */
extern int hashmap_get(map_t in, char* key, any_t *arg);

/*
 * Remove an element from the hashmap. Return MAP_OK or MAP_MISSING.
 */
extern int hashmap_remove(map_t in, char* key);

/*
 * Get any element. Return MAP_OK or MAP_MISSING.
 * remove - should the element be removed from the hashmap
 */
extern int hashmap_get_one(map_t in, any_t *arg, int remove);

/*
 * Free the hashmap
 */
extern void hashmap_free(map_t in);

/*
 * Get the current size of a hashmap
 */
extern int hashmap_length(map_t in);

//#endif __HASHMAP_H__
#endif


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// aux
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef ___GRT_AUX___
#define ___GRT_AUX___

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libgen.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// binary file and related methods

unsigned long hash_djb2(unsigned char *str);

char* read_binary_file(char* filename, long* size, int* error);

void read_binary_file_size(char* filename, long* size, int* error);

void write_binary_file(char* filename, char* source, int* error);

void write_raw_file(char* filename, float* source, size_t len, int* error);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// envelope methods

#define ENVELOPE_FORMAT "%s\07%s" // delimiter is \07 ascii beep :D
#define ENVELOPE_FORMAT_DELIMITER "\07"

typedef struct {
    char *name;
    char *source;
} envelope_t;

envelope_t* envelope_create(char* name, char* source);

void envelope_destroy(envelope_t* env);

void envelope_pack(envelope_t* envelope, char** arg);

void envelope_unpack(char* envelope, envelope_t** arg);

void envelope_print(envelope_t* env);

void envelope_writeout(envelope_t* env, char* path, int* error);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// rx frame handling: chunks, transmits

typedef struct {
    unsigned int num;
    char* data;
    size_t len;
} chunk_t;

typedef struct {
    unsigned long hash;
    unsigned int max;
    chunk_t* chunks;
} transmit_t;

transmit_t* transmit_create(unsigned long hash, unsigned int max);

void transmit_destroy(transmit_t* transm);

void transmit_add(transmit_t* transm, unsigned int num, char* buffer, size_t buffer_len);

bool transmit_is_complete(transmit_t* transm);

void transmit_concatenate(transmit_t* transm, char** arg);

void transmit_print(transmit_t* transm);

void transmit_get_envelope(transmit_t* transm, envelope_t** arg);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// rx frame handler

#define RXMAP_KEY_LEN 64
#define RXMAP_KEY_FORMAT "%lu"

typedef struct {
    map_t* transmits;
} rxhandler_t;

rxhandler_t* rxhandler_create();

void rxhandler_destroy(rxhandler_t* rxm);

void rxhandler_add(rxhandler_t* rxm, unsigned long hash, unsigned int num, unsigned int max, char* buffer, size_t buffer_len);

void rxhandler_remove(rxhandler_t* rxm, unsigned long hash);

void rxhandler_get(rxhandler_t* rxm, unsigned long hash, transmit_t** arg);

void rxhandler_reap(rxhandler_t* rxm, transmit_t** ripe);

typedef void list_cb_t(transmit_t* transm, void* user);

void rxhandler_list(rxhandler_t* rxm, list_cb_t* callback, void* user);

#endif


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// modem
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//

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

grtModemOpt_t* grtModemOpt_parse_args_from_file(char* filename, bool is_tx); 

grtModemOpt_t* grtModemOpt_parse_args(int argc, char** argv, bool is_tx);

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
    grtModemRX_emit_debug_callback *emit_debug_callback;
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




#endif
