/*
 * Gretchen
 */

#pragma once

#define gretchen_VERSION_MAJOR 0
#define gretchen_VERSION_MINOR 2

#include "gretchen.internal.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TX

typedef struct {
    grtModemTX_t* modem_tx;
    uint8_t* packed_env;
    float* samples;
    size_t samples_len;
} gretchenTX_t;

typedef struct {
    size_t filesize_bytes;
    size_t est_encodedsize_samples;
    size_t est_transfer_sec;
} gretchenTX_inspect_t;

/*
 * Creates the sender modem from options `opt` and with an internal ringbuffer
 * of size of `internal_bufsize`. 
 */
gretchenTX_t* gretchenTX_create(grtModemOpt_t* opt, size_t internal_bufsize);

/*
 * Destroys the sender modem. 
 */
void gretchenTX_destroy(gretchenTX_t* tx);

/*
 * Inspects the given file in `filename` and estimates the duration and size
 * of the resulting transmission. The generated information can be accessed 
 * via the parameter `info`. 
 * NOTE the file is not loaded in memory.
 */
void gretchenTX_inspect(gretchenTX_t* tx, uint8_t* filename, int8_t* error, gretchenTX_inspect_t** info);

/*
 * Prepares aka encodes the file in `filename` for transmission. The coded
 * data (a long audio sample) is stored by the sender modem internally and 
 * is accessible by calling the `get` method.
 */
void gretchenTX_prepare(gretchenTX_t* tx, uint8_t* filename, int8_t* error);

/*
 * Copies the coded data (audio sample) into given `samplebuffer` location. Its size is 
 * written into parameter `len`.
 */
void gretchenTX_get(gretchenTX_t* tx, float** samplebuffer, size_t* len);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// RX

/*
 * The filecomplete callback is called when a file has been sucessfully 
 * extracted from the audio stream (all frames have been received).
 */
typedef void gretchenRX_filecomplete_callback(uint8_t* filename, uint8_t* source, size_t sourcelen, void* user);

/*
 * The progress callback is called each time a frame is decoded with at 
 * least its header intact.
 */
typedef void gretchenRX_progress_callback(uint64_t hash, uint32_t frame_num, uint32_t frame_nummax, int payload_valid, void* user);

typedef struct {
    grtModemRX_t* modem_rx;
    rxhandler_t* rxhandler;
    gretchenRX_filecomplete_callback* callback;
    gretchenRX_progress_callback* prog_callback;
    void* callbackuser;
} gretchenRX_t;

/*
 * Creates the receiver modem from options `opt` and with an internal ringbuffer
 * of size of `internal_bufsize`. 
 */
gretchenRX_t* gretchenRX_create(grtModemOpt_t* opt, size_t internal_bufsize);

/*
 * Destroys the reciever modem.
 */
void gretchenRX_destroy(gretchenRX_t* rx);

/*
 * Pushes a chunk of (recorded) samples in `buffer` of size `len` into the 
 * modem. The assumed format is little endian, 16 bit, float (le16f).
 */
void gretchenRX_push_le16f(gretchenRX_t* rx, float* buffer, size_t len, int8_t* error);



