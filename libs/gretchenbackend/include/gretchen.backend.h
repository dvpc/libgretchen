/*
 * Gretchen Backend
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SIGNAL CATCHING
/*
catching SIGINT (ctrl-c)
probably only posix (no windows i guess)
TODO: check and rewrite on windows!
we will abstract this part out anyway so no problem.

the thing is that calling most functions from inside a handler is unsafe!
so i just set a global variable inside the handler an do the check in the 
main function.

https://stackoverflow.com/questions/1641182/how-can-i-catch-a-ctrl-c-event-c
https://stackoverflow.com/questions/5378778/what-does-d-xopen-source-do-mean

signals are bad one should use sigaction!
http://pubs.opengroup.org/onlinepubs/9699919799/functions/sigaction.html

another way can be this:
http://zguide.zeromq.org/c:interrupt

TODO: can it fail?? How to know?? Do we need to know anyway?
*/

#define _XOPEN_SOURCE 500
#include <signal.h>
#include <unistd.h>

typedef struct {
    int grtSigcatcher_should_terminate;
} sigcatcher_t;

void grtSigcatcher_handler(int s);

void grtSigcatcher_Init();

int grtSigcatcher_ShouldTerminate();

void grtSigcatcher_Set(int i);

void grtSigcatcher_Destroy();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// (PORT)AUDIO BACKEND
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <portaudio.h>
#include <liquid/liquid.h>

typedef struct {
    bool is_tx;
    PaError err;
    PaStream* stream;
    PaStreamParameters strParams;
    cbufferf samplebuffer;
    uint32_t samplerate;
} grtBackend_t;

void grtBackend_estimate_inputdecive_numchannels(PaDeviceIndex device, int* result_num_channel, int8_t* error, uint32_t samplerate);

grtBackend_t* grtBackend_create(size_t internalbufsize, bool is_tx, uint32_t samplerate);

void grtBackend_destroy(grtBackend_t* back);

void grtBackend_startstream(grtBackend_t* back, int8_t* error);

bool grtBackend_isstreamactive(grtBackend_t* back);

const uint8_t* grtBackend_getstatustext(grtBackend_t* back);

void grtBackend_stopstream(grtBackend_t* back, int8_t* error);

size_t grtBackend_push_available(grtBackend_t* back);

size_t grtBackend_push(grtBackend_t* back, float* buffer, size_t len);

void grtBackend_poll(grtBackend_t* back, size_t ask, float** buffer, size_t* len);
        
