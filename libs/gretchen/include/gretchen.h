/*
 * Gretchen
 * transmit and receive modules
 */
#define gretchen_VERSION_MAJOR @gretchen_VERSION_MAJOR@
#define gretchen_VERSION_MINOR @gretchen_VERSION_MINOR@
// TODO
// sampling rate ?? only in options.c...
// would also need to add a resampler!!! but np!!
// 
// FIXME
// How to indicate that clearly??
// All internal raw sample data is float 16 bit le
// 44100 hz
//
#ifndef ___GRETCHEN_H___
#define ___GRETCHEN_H___

#include "internal/grt.h"
#include "internal/grt_aux.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TX

// FIXME
// maybe better to define a max transfer time???
// maybe ok but better to parameterize this...
#define MAX_SUGGESTED_FILESIZE 1 << 20 // 1048576 bytes ~ 1 MB

typedef struct {
    grtModemTX_t* modem_tx;
    char* packed_env;
    float* samples;
    size_t samples_len;
} gretchenTX_t;

gretchenTX_t* gretchenTX_create(grtModemOpt_t* opt, int internal_bufsize);

void gretchenTX_destroy(gretchenTX_t* tx);

typedef struct {
    int is_toolarge;
    size_t filesize_bytes;
    size_t est_encodedsize_samples;
    size_t est_transfer_sec;
} gretchenTX_inspect_t;

void gretchenTX_inspect(gretchenTX_t* tx, char* filename, int* error, gretchenTX_inspect_t** info);

void gretchenTX_prepare(gretchenTX_t* tx, char* filename, int* error);

void gretchenTX_get(gretchenTX_t* tx, float** samplebuffer, size_t* len);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// RX

typedef void gretchenRX_filecomplete_callback(char* filename, char* source, size_t sourcelen, void* user);

typedef void gretchenRX_progress_callback(unsigned long hash, unsigned int frame_num, unsigned int frame_nummax, void* user);

typedef struct {
    grtModemRX_t* modem_rx;
    rxhandler_t* rxhandler;
    gretchenRX_filecomplete_callback* callback;
    gretchenRX_progress_callback* prog_callback;
    void* callbackuser;
} gretchenRX_t;

gretchenRX_t* gretchenRX_create(grtModemOpt_t* opt, int internal_bufsize);

void gretchenRX_destroy(gretchenRX_t* rx);

void gretchenRX_push(gretchenRX_t* rx, float* buffer, size_t len, int* error);


#endif


