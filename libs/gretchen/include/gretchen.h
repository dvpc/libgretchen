/*
 * Gretchen
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

#pragma once

#define gretchen_VERSION_MAJOR 0
#define gretchen_VERSION_MINOR 3

#include "gretchen.internal.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TX

/*
 * This optional callback is called each time a frame is encoded. 
 * The number of encoded samples so far is reported.
 * Its role is to notify a possible user interface. 
 */
typedef void gretchenTX_progress_callback(size_t currentbuflen, void* user);

typedef struct {
    grtModemTX_t* modem_tx;
    uint8_t* packed_env;
    float* samples;
    size_t samples_len;
    gretchenTX_progress_callback* prog_callback;
    void* callbackuser;
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
 * Inspects the given buffer and estimates the duration and size
 * of the resulting transmission. The generated information can be accessed
 * via the parameter `info`.
 */
void gretchenTX_inspect_buffer(gretchenTX_t* tx, uint8_t* buffer, gretchenTX_inspect_t** info);
/*
 * Prepares aka encodes the file in `filename` for transmission. The coded
 * data (a long audio sample) is stored by the sender modem internally and 
 * is accessible by calling the `get` method.
 */
void gretchenTX_prepare(gretchenTX_t* tx, uint8_t* filename, int8_t* error);
/*
 * Prepares aka encodes the given buffer for transmission. The coded data
 * (a long audio sample) is stored by the sender modem internally and
 * is accessible by calling the `get` method.
 */
void gretchenTX_prepare_buffer(gretchenTX_t* tx, uint8_t* buffer);
/*
 * Copies the coded data (audio sample) into given `samplebuffer` location. Its size is 
 * written into parameter `len`.
 */
void gretchenTX_get(gretchenTX_t* tx, float** samplebuffer, size_t* len);
/*
 * Sets the progress callback function pointer.
 */
void gretchenTX_set_progress_cb(gretchenTX_t* tx, gretchenTX_progress_callback* cb);
/*
 * Sets the userdara pointer for the callback functions.
 */
void gretchenTX_set_callback_userdata(gretchenTX_t* rx, void* user);

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
 * The paramteter `payload_valid` indicates if the payload has been decoded sucessfully or not.
 */
typedef void gretchenRX_progress_callback(uint16_t hash, uint16_t frame_num, uint16_t frame_nummax, int payload_valid, void* user);

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
/*
 * Sets the filecomplete callback function pointer.
 */
void gretchenRX_set_filecomplete_cb(gretchenRX_t* rx, gretchenRX_filecomplete_callback* cb);
/*
 * Sets the progress callback function pointer.
 */
void gretchenRX_set_progress_cb(gretchenRX_t* rx, gretchenRX_progress_callback* cb);
/*
 * Sets the userdara pointer for the callback functionss.
 */
void gretchenRX_set_callback_userdata(gretchenRX_t* rx, void* user);
/*
 * Sets the optional modemRX debug callback function pointer.
 * See gretchen.internal.h
 */
void gretchenRX_set_debug_cb(gretchenRX_t* rx, grtModemRX_emit_debug_callback* cb);
