#include "gretchen.backend.h"

static int playCallback(
                const void *inbuf, 
                void *outbuf,
                unsigned long frmsPerBuf, 
                const PaStreamCallbackTimeInfo* timeInfo,
                PaStreamCallbackFlags statFlags,
                void *userdata) 
{
    (void) inbuf;
    (void) statFlags;
    (void) timeInfo;
    grt_playback *bplay = (grt_playback*)userdata;

    float *outp = (float*)outbuf;
    float *samples;
    unsigned int nread;
    cbufferf_read(bplay->play_cb, frmsPerBuf, &samples, &nread);
    cbufferf_release(bplay->play_cb, nread);
    for (size_t k=0; k<nread; k++)
        outp[k] = samples[k];
    /*for (size_t k=0; k<frmsPerBuf; k++) {*/
        /**outp++ = 0.0f;*/
        /**outp++ = samples[k];*/
    /*}*/
    return paContinue;
}

grt_playback* grt_playback_Create(
                size_t internal_rbuf_len) 
{
    grt_playback* bplay = malloc(sizeof(grt_playback));
    bplay->err = Pa_Initialize();
    if (bplay->err!=paNoError) {
        bplay->cb_err = grtbckErrorInit;
        return NULL;
    }
    bplay->outpParams.device = Pa_GetDefaultOutputDevice();
    if (bplay->outpParams.device==paNoDevice) {
        bplay->cb_err = grtbckErrorNoPlayDevice;
        return NULL;
    }
    bplay->outpParams.channelCount = NUM_CHANNELS;
    bplay->outpParams.sampleFormat = paFloat32; 
    bplay->outpParams.suggestedLatency = Pa_GetDeviceInfo(
                    bplay->outpParams.device)->defaultLowOutputLatency;
    bplay->outpParams.hostApiSpecificStreamInfo = NULL;

    size_t rbufsize = internal_rbuf_len < MIN_RINGBUF_SIZE ? 
            MIN_RINGBUF_SIZE : internal_rbuf_len;
    bplay->play_cb = cbufferf_create(rbufsize);
    return bplay;
}

grtbckError grt_playback_StartStream(
                grt_playback* bplay)
{
    bplay->err = Pa_OpenStream(
                    &bplay->stream,
                    NULL,
                    &bplay->outpParams,
                    SAMPLE_RATE,
                    paFramesPerBufferUnspecified,
                    paNoFlag,
                    playCallback,
                    bplay); 
    if (bplay->err != paNoError) {
        bplay->cb_err = grtbckErrorInitStream;
        return bplay->cb_err;    
    }
    bplay->err = Pa_StartStream(bplay->stream);
    if (bplay->err!=paNoError) {
        bplay->cb_err= grtbckErrorOpenStream;
        return bplay->cb_err;    
    }
    bplay->cb_err = grtbckNoError;
    return bplay->cb_err; 
}

size_t grt_playback_PushAvailable(
                grt_playback* bplay)
{
    return cbufferf_max_size(bplay->play_cb) - 
           cbufferf_size(bplay->play_cb);
}

size_t grt_playback_PushNonBlocking(
                grt_playback* bplay,
                float* ibuffer,
                size_t len)
{
    size_t avail = cbufferf_max_size(bplay->play_cb) - 
                       cbufferf_size(bplay->play_cb);
    if (avail==0)
        return 0;
    size_t num = avail < len ? avail : len;
    cbufferf_write(bplay->play_cb, ibuffer, num); 
    return num;
}

grtbckError grt_playback_StreamStatus(
                grt_playback* bplay)
{
    return bplay->cb_err;
}

grtbckError grt_playback_StopStreaming(
                grt_playback* bplay) 
{
    if (bplay->stream) {
        bplay->err = Pa_StopStream(bplay->stream);
        bplay->err = Pa_AbortStream(bplay->stream);
        if (bplay->err!=paNoError) {
            bplay->cb_err = grtbckErrorCloseStream;
            return bplay->cb_err; 
        }
    }
    return grtbckNoError;
}

size_t grt_playback_PollAvailableSamples(
                grt_playback* bplay)
{
    return cbufferf_size(bplay->play_cb);
}

void grt_playback_Destroy(
                grt_playback* bplay)
{
    if (bplay) {
        bplay->err = Pa_Terminate();
        cbufferf_destroy(bplay->play_cb);
        free(bplay);
    }
}





