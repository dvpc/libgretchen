/*
 * Gretchen
 */
// TODO
// PUT the following in the documentation:
// All internal raw sample data is assumed to be 
// float 16 bit le with a now variable samplerate.

#pragma once

#define gretchen_VERSION_MAJOR 0
#define gretchen_VERSION_MINOR 1

#include "gretchen.internal.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TX

typedef struct {
    grtModemTX_t* modem_tx;
    uint8_t* packed_env;
    float* samples;
    size_t samples_len;
} gretchenTX_t;

gretchenTX_t* gretchenTX_create(grtModemOpt_t* opt, size_t internal_bufsize);

void gretchenTX_destroy(gretchenTX_t* tx);

typedef struct {
    size_t filesize_bytes;
    size_t est_encodedsize_samples;
    size_t est_transfer_sec;
} gretchenTX_inspect_t;

void gretchenTX_inspect(gretchenTX_t* tx, uint8_t* filename, int8_t* error, gretchenTX_inspect_t** info);

void gretchenTX_prepare(gretchenTX_t* tx, uint8_t* filename, int8_t* error);

void gretchenTX_get(gretchenTX_t* tx, float** samplebuffer, size_t* len);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// RX

typedef void gretchenRX_filecomplete_callback(uint8_t* filename, uint8_t* source, size_t sourcelen, void* user);

typedef void gretchenRX_progress_callback(uint64_t hash, uint32_t frame_num, uint32_t frame_nummax, int payload_valid, void* user);

typedef struct {
    grtModemRX_t* modem_rx;
    rxhandler_t* rxhandler;
    gretchenRX_filecomplete_callback* callback;
    gretchenRX_progress_callback* prog_callback;
    void* callbackuser;
} gretchenRX_t;

gretchenRX_t* gretchenRX_create(grtModemOpt_t* opt, size_t internal_bufsize);

void gretchenRX_destroy(gretchenRX_t* rx);

void gretchenRX_push_le16f(gretchenRX_t* rx, float* buffer, size_t len, int8_t* error);



