#ifndef GRT_BACKEND
#define GRT_BACKEND

#include <stdio.h>
#include <stdlib.h>

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


#endif
