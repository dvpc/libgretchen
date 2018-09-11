/*
 * Gretchen internal
 */

#pragma once

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ringbuffer
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/* 
 * Simple ring buffer with memcpy
 * not thread safe! Ugly and simple :D
 */

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
    uint8_t haslockpop;
    uint8_t haslockpush;
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
extern int8_t hashmap_iterate(map_t in, PFany f, any_t item);

/*
 * Add an element to the hashmap. Return MAP_OK or MAP_OMEM.
 */
extern int8_t hashmap_put(map_t in, uint8_t* key, any_t value);

/*
 * Get an element from the hashmap. Return MAP_OK or MAP_MISSING.
 */
extern int8_t hashmap_get(map_t in, uint8_t* key, any_t *arg);

/*
 * Remove an element from the hashmap. Return MAP_OK or MAP_MISSING.
 */
extern int8_t hashmap_remove(map_t in, uint8_t* key);

/*
 * Get any element. Return MAP_OK or MAP_MISSING.
 * remove - should the element be removed from the hashmap
 */
extern int8_t hashmap_get_one(map_t in, any_t *arg, uint8_t remove);

/*
 * Free the hashmap
 */
extern void hashmap_free(map_t in);

/*
 * Get the current size of a hashmap
 */
extern int32_t hashmap_length(map_t in);



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// aux
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libgen.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// binary file and related methods

/*
 * A hashfunction by Daniel Bernstein.
 * http://www.cse.yorku.ca/~oz/hash.html
 */
uint64_t hash_djb2(uint8_t *str);

/*
 * Reads a file and returns the location of the file contents as bytes.
 * The parameter `size` will be set to the actual number of bytes read. 
 */
uint8_t* read_binaryfile(uint8_t* filename, int64_t* size, int8_t* error);

/*
 * Optains the size of the given file (which is NOT loaded in memory).
 * The parameter `size` will be set to the actual number of bytes obtained.
 */
void optain_binaryfile_size(uint8_t* filename, int64_t* size, int8_t* error);

/*
 * Writes `size` bytes from `source` into given file.
 */
void write_binaryfile(uint8_t* filename, uint8_t* source, int8_t* error);

/*
 * Writes a pcm coded raw audio file (without header).
 */
void write_rawfile(uint8_t* filename, float* source, size_t len, int8_t* error);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// envelope methods

#define ENVELOPE_FORMAT "%s\07%s" // delimiter is \07 ascii beep :D
#define ENVELOPE_FORMAT_DELIMITER "\07"

typedef struct {
    uint8_t *name;
    uint8_t *source;
} envelope_t;

/*
 * Creates an envelope with `name` and bytes from `source`.
 * The size to store `source` is derived by strlen! 
 * So the data at `source` needs to be \0 terminated. 
 * This is ensured by the `read_binaryfile` method.
 */
envelope_t* envelope_create(uint8_t* name, uint8_t* source);

/*
 * Destroys the envelope.
 */
void envelope_destroy(envelope_t* env);

/*
 * Packs the envelope aka - writes a combined string into the `arg`
 * location, which cointains the envelopes (file)name and its (file)data.
 */
void envelope_pack(envelope_t* envelope, uint8_t** arg);

/*
 * Unpacks a combined string into an envelope which is written to 
 * location of `arg`.
 */
void envelope_unpack(uint8_t* envelope, envelope_t** arg);

/*
 * Prints the envelope to stdout (printf).
 */
void envelope_print(envelope_t* env);

/*
 * Writes the contents of the envelope as a file into given `path` using
 * the name of the envelope as filename and the contents as data.
 */
void envelope_writeout(envelope_t* env, uint8_t* path, int8_t* error);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// rx frame handling: chunks, transmits

typedef struct {
    uint32_t num;
    uint8_t* data;
    size_t len;
} chunk_t;

typedef struct {
    uint64_t hash;
    uint32_t max;
    chunk_t* chunks;
} transmit_t;

/*
 * Creates a transmit. A transmit object holds all recieved frames together.
 */
transmit_t* transmit_create(uint64_t hash, uint32_t max);

/*
 * Destroys the transmit.
 */
void transmit_destroy(transmit_t* transm);

/*
 * Add the payload of a frame (the `buffer`) to the transmit.
 */
void transmit_add(transmit_t* transm, uint32_t num, uint8_t* buffer, size_t buffer_len);

/*
 * Returns true if all chunks have been added to the transmit. False otherwise.
 */
bool transmit_is_complete(transmit_t* transm);

/*
 * Prints the transmit to stdout.
 */
void transmit_print(transmit_t* transm);

/*
 * Creates an envelope object from the chunks in the location of `arg`.
 */
void transmit_get_envelope(transmit_t* transm, envelope_t** arg);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// rx frame handler

#define RXMAP_KEY_LEN 64
#define RXMAP_KEY_FORMAT "%lu"

typedef struct {
    map_t* transmits;
} rxhandler_t;

typedef void list_cb_t(transmit_t* transm, void* user);

/*
 * Creates a receive (==rx) handler, which takes care of transmit objects.
 */
rxhandler_t* rxhandler_create();

/*
 * Destroy the rxhandler.
 */
void rxhandler_destroy(rxhandler_t* rxm);

/*
 * Adds the extracted payload and hash of a frame to the rxhandler.
 * Note, if no transmit exists for given hash a new transmit object is 
 * created. 
 */
void rxhandler_add(rxhandler_t* rxm, uint64_t hash, uint32_t num, uint32_t max, uint8_t* buffer, size_t buffer_len);

/*
 * Removes transmit with given `hash` from handlers internal hashmap.
 */
void rxhandler_remove(rxhandler_t* rxm, uint64_t hash);

/*
 * Gives access to transmit object of given `hash` at location `arg`.
 */
void rxhandler_get(rxhandler_t* rxm, uint64_t hash, transmit_t** arg);

/*
 * Gives access to first complete transmit of the handler located in `ripe`.
 * `ripe` is NULL if there is no complete transmit or any at all.
 * It is assumed to later remove the complete transmit from the handler.
 */
void rxhandler_reap(rxhandler_t* rxm, transmit_t** ripe);

/*
 * Gives a `callback` method which is called for each transmit object in the handler.
 * An additional `user` pointer my be given to access other parts in the 
 * callbackfunction.
 */
void rxhandler_list(rxhandler_t* rxm, list_cb_t* callback, void* user);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// modem
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//

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
    size_t payload_len;
    crc_scheme checksum_scheme;
    fec_scheme inner_fec_scheme;
    fec_scheme outer_fec_scheme;
    modulation_scheme mod_scheme;
    uint32_t _bits_per_symbol;
} grtFrameOpt_t;

typedef struct {
    uint32_t shape;  
    uint32_t samples_per_symbol;
    uint32_t symbol_delay;
    float excess_bw;
    float center_rads;
    float gain;
    uint32_t flushlen_mod;
    uint32_t txflt_order;
    float txflt_cutoff_frq;
    float txflt_center_frq;
    uint32_t rxflt_order;
    float rxflt_cutoff_frq;
    float rxflt_center_frq;
} grtModulatorOpt_t;

typedef struct {
    uint32_t num_subcarriers;
    uint32_t cyclic_prefix_len;
    uint32_t taper_len;
    size_t left_band;
    size_t right_band;
} grtOfdmOpt_t;

typedef struct {
    grtFrameType_t frametype;
    grtFrameOpt_t *frameopt;
    grtModulatorOpt_t *modopt;
    grtOfdmOpt_t *ofdmopt;
    uint32_t samplerate;
} grtModemOpt_t;

/*
 * Creates a modem option object containing all possible parameters.
 * Heavily uses methods from liquid sdr to parse the actual string parameters.
 * The `samplerate` is given seperately here. 
 */
grtModemOpt_t* grtModemOpt_create_default(uint32_t samplerate);

/*
 * Creates a modem option object from a file.
 */
grtModemOpt_t* grtModemOpt_parse_args_from_file(uint8_t* filename, bool is_tx, uint32_t samplerate); 

/*
 * Creates a modem option object from command line arguments.
 */
grtModemOpt_t* grtModemOpt_parse_args(int argc, char** argv, bool is_tx, uint32_t samplerate);

/*
 * Destroys the modem option object.
 */
void grtModemOpt_destroy(grtModemOpt_t* opt);

/*
 * Prints the contents of the modem option object.
 */
void grtModemOpt_print(grtModemOpt_t* opt);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Modem 

#define MODEM_HEADER_LEN 2+sizeof(uint64_t)*3 
#define MODEM_HEADER_FORMAT "%lu,%d,%d"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ModulatorRX (Demodulator)
 
typedef struct {
    nco_crcf nco;
    firdecim_crcf decim;
    iirfilt_rrrf filter_rx;
    uint32_t samples_per_symbol;
    agc_rrrf agc;
    iirfilt_crcf dcfilter; 
} grtModulatorRX_t;

/*
 * Creates the demodulator object, which is demodulating real valued streams 
 * to complex ones.
 */
grtModulatorRX_t *grtModulatorRX_create(uint32_t shape, uint32_t samples_per_symbol, uint32_t symbol_delay, float excess_bw, float center_rads, uint32_t flt_order, float flt_cutoff_frq, float flt_center_frq, float flt_passband_ripple, float flt_stopband_ripple);

/*
 * Destroys the demodulator.
 */
void grtModulatorRX_destroy(grtModulatorRX_t *dem);

/*
 * Receives a real valued chunk `samples` and writes a complex chunk into 
 * `symbols`, the number of symbols written is returned.
 */
size_t grtModulatorRX_recv(grtModulatorRX_t *dem, float *samples, size_t samples_len, float complex *symbols);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ModemRX (Decoder)

/*
 * Frame successfully received callback - if a frame has been sucessfully 
 * decoded this callback is called with the data of the frame and additional
 * header info. 
 */
typedef void grtModemRX_emit_callback(uint64_t hash, uint32_t frame_num, uint32_t frame_nummax, size_t buffer_len, uint8_t *buffer, void *user);

/*
 * Progress callback is called when a frame is decoded (at least the header has 
 * to be successful decoded), no matter if the payload is valid or not.
 */
typedef void grtModemRX_emit_progress_callback(uint64_t hash, uint32_t frame_num, uint32_t frame_nummax, int payload_valid, void *user);

/*
 * Debug callback fires always and gives additional detailed statistics 
 * from liquid sdr.
 */
typedef void grtModemRX_emit_debug_callback(int header_valid, int payload_valid, uint32_t payload_len, framesyncstats_s stats);

typedef struct { 
    flexframesync framesync; 
} modem_decoder_t;

typedef struct { 
    gmskframesync framesync; 
} gmsk_decoder_t;

typedef struct {
    ofdmflexframesync framesync;
} ofdm_decoder_t;

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
        ofdm_decoder_t ofdm;
    } frame;
    grtModulatorRX_t *dem;

    float complex* symbolbuf;
    size_t symbolbuf_len;

    cbufferf consume_cb;

    grtModemRX_emit_callback *emit_callback;
    grtModemRX_emit_progress_callback *emit_progress_callback;
    grtModemRX_emit_debug_callback *emit_debug_callback;
    void *emit_callback_userdata;
} grtModemRX_t;

/*
 * Creates a receiving modem object from options `opt` with an internal
 * ringbuffer of size `internal_bufsize`.
 */
grtModemRX_t *grtModemRX_create(const grtModemOpt_t *opt, size_t internal_bufsize);

/*
 * Destroys the receiving modem object.
 */
void grtModemRX_destroy(grtModemRX_t *mrx);

/*
 * Sets the modem in `flush` mode, which causes the modem to create a new 
 * frame each time new data is consumed. Normal behavior is to wait until 
 * the given frame length is reached to create a new frame.
 */
void grtModemRX_enable_flush(grtModemRX_t *mrx);

/*
 * Resets the modem. If the modem in flush mode it is set to normal 
 * operation mode. Also the internal ringbuffer is reset. 
 */
void grtModemRX_reset(grtModemRX_t *mrx);

/*
 * Consumes (read) a given chunk of real valued samples in `buffer` of 
 * given size in `buflen`.
 */
size_t grtModemRX_consume(grtModemRX_t *mrx, float *buffer, size_t buflen);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ModulatorTX 

typedef struct {
    nco_crcf nco;
    firinterp_crcf interp;
    //iirfilt_crcf filter_tx;
    uint32_t samples_per_symbol;
    float gain;
    size_t flushlen;
} grtModulatorTX_t;

/*
 * Creates a modulator object which is modulating complex symbols into real 
 * valued samples. 
 */
grtModulatorTX_t *grtModulatorTX_create(uint32_t shape, uint32_t samples_per_symbol, uint32_t symbol_delay, float excess_bw, float center_rads, float gain, uint32_t flt_order, float flt_cutoff_frq, float flt_center_frq, float flt_passband_ripple, float flt_stopband_ripple, uint32_t flushlen_mod);

/*
 * Destroys the modulator object.
 */
void grtModulatorTX_destroy(grtModulatorTX_t *mod);

/*
 * Receives a complex valued chunk `symbols` and writes a real valued chunk into 
 * `samples`, the number of samples written is returned.
 */
size_t grtModulatorTX_recv(grtModulatorTX_t *mod, float complex *symbols, size_t symbols_len, float *samples);

/*
 * Creates an empty chunk of sample data to write gaps between frames.
 */
size_t grtModulatorTX_flush(grtModulatorTX_t *mod, float *samples);

/*
 * Resets the modulators interpolation filter.
 */
void grtModulatorTX_reset(grtModulatorTX_t *mod);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ModemTX (Encoder) 

/*
 * Successful frame generated callback.
 * Is called when a new frame of samples has been generated of
 * the consumed input.
 */
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
    ofdmflexframegen framegen;
} ofdm_encoder_t;

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
        ofdm_encoder_t ofdm;
    } frame;
    grtModulatorTX_t *mod;

    rbufu_t* cons_rbuf;
    rbufError cons_rbuf_err;

    uint8_t *buf_readframe;
    float *buf_samples;
    float *buf_flush;
    
    grtModemTX_emit_callback *emit_callback;
    void *emit_callback_userdata;

    uint64_t hash;
    uint32_t frame_num;
    uint32_t frame_nummax;
} grtModemTX_t;

/*
 * Creates a sending modem object.
 */ 
grtModemTX_t *grtModemTX_create(const grtModemOpt_t *opt, size_t internal_bufsize);

/*
 * Destroys the sending modem object.
 */
void grtModemTX_destroy(grtModemTX_t *mtx);

/*
 * Sets the header information: `hash` and the `filesize`.
 * These will be written as additional header information.
 */
void grtModemTX_setheaderinfo(grtModemTX_t *mtx, uint64_t filehash, size_t filesize); 

/*
 * Sets the modem in `flush` mode, which causes the modem to create a new 
 * frame each time new data is consumed. Normal behavior is to wait until 
 * the given frame length is reached to create a new frame.
 */
void grtModemTX_enable_flush(grtModemTX_t *mtx);

/*
 * Resets the modem. If the modem in flush mode it is set to normal 
 * operation mode. Also the internal ringbuffer is reset. 
 */
void grtModemTX_reset(grtModemTX_t *mtx);

/*
 * Consumes (read) a given chunk of bytes in `buffer` of 
 * given size in `buflen`.
 */
size_t grtModemTX_consume(grtModemTX_t *mtx, const void *buffer, size_t buflen);

/*
 * Calculate the number of symbols needed for a given number of 
 * bytes.
 */
size_t framegen_estimate_num_symbols(grtModemTX_t *mtx, size_t len);



