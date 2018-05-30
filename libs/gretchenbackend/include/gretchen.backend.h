/*
 * Gretchen Backend
 */
#ifndef GRT_BACKEND
#define GRT_BACKEND

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
    int grt_sigcatch_should_terminate;
} sigcatcher_t;

void grt_sigcatch_handler(int s);

void grt_sigcatch_Init();

int grt_sigcatch_ShouldTerminate();

void grt_sigcatch_Set(int i);

void grt_sigcatch_Destroy();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AUDIO BACKEND
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// FIXME
// will be factored out!!!!!

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1
#define MIN_RINGBUF_SIZE 4096

// FIXME
// factor this out from here :D
#include <portaudio.h>
#include <liquid/liquid.h>

// FIXME
// i should just propagate the backend error!!!
//
typedef enum grtbckError {
    grtbckNoError = 0,
    grtbckErrorInit = -1,
    grtbckErrorNoRecDevice = -2,
    grtbckErrorNoPlayDevice = -3,
    grtbckErrorInitStream = -4,
    grtbckErrorOpenStream = -5,
    grtbckErrorStopStream = -6,
    grtbckErrorCloseStream = -7,
    grtbckErrorBufferOverrun = -8,
    grtbckErrorBufferUnderrun = -9
} grtbckError;

typedef struct {
    PaError err;
    PaStream* stream;
    PaStreamParameters outpParams;

    cbufferf play_cb;

    grtbckError cb_err;
} grt_playback;

grt_playback* grt_playback_Create();

grtbckError grt_playback_StartStream(grt_playback* bplay);

size_t grt_playback_PushAvailable(grt_playback* bplay);

size_t grt_playback_PushNonBlocking(grt_playback* bplay, float* ibuffer, size_t len);

grtbckError grt_playback_StreamStatus(grt_playback* bplay);

// FIXME unused
size_t grt_playback_PollAvailableSamples(grt_playback* bplay);

grtbckError grt_playback_StopStreaming(grt_playback* bplay);

void grt_playback_Destroy(grt_playback* bplay);

typedef struct {
    PaError err;
    PaStream* stream;
    PaStreamParameters inpParams;

    cbufferf rec_cb;
    
    grtbckError cb_err;
} grt_record;


grt_record* grt_record_Create(size_t internal_rbuf_len);

grtbckError grt_record_StartStream(grt_record* brec);

size_t grt_record_PollNonBlocking(grt_record* brec, float* obuffer, size_t len);

// FIXME unused
grtbckError grt_record_StreamStatus(grt_record* brec);

// FIXME unused
size_t grt_record_PollAvailableSamples(grt_record* brec);

grtbckError grt_record_StopStreaming(grt_record* brec);

void grt_record_Destroy(grt_record* brec);




typedef struct {
    bool is_tx;
    PaError err;
    PaStream* stream;
    PaStreamParameters strParams;
    cbufferf samplebuffer;
} grtBackend_t;

grtBackend_t* grtBackend_create(size_t internalbufsize, bool is_tx);

void grtBackend_destroy(grtBackend_t* back);

void grtBackend_startstream(grtBackend_t* back, int* error);

bool grtBackend_isstreamactive(grtBackend_t* back);

void grtBackend_stopstream(grtBackend_t* back, int* error);

size_t grtBackend_push_available(grtBackend_t* back);

size_t grtBackend_push(grtBackend_t* back, float* buffer, size_t len);

void grtBackend_poll(grtBackend_t* back, size_t ask, float** buffer, size_t* len);

















#endif
