#include "gretchen.backend.h"

static int recCallback(
                const void *inbuf, 
                void *outbuf,
                unsigned long frmsPerBuf, 
                const PaStreamCallbackTimeInfo* timeInfo,
                PaStreamCallbackFlags statFlags,
                void *userdata) 
{
    (void) outbuf;
    (void) statFlags;
    (void) timeInfo;
    grt_record *brec = (grt_record*)userdata;

    float *inp = (float*)inbuf;
    size_t avail = cbufferf_max_size(brec->rec_cb) - 
                       cbufferf_size(brec->rec_cb);
    if (avail < frmsPerBuf) {
        brec->cb_err = grtbckErrorBufferOverrun;
        return paComplete;
    }
    cbufferf_write(brec->rec_cb, inp, frmsPerBuf); 
    return paContinue;
}

grt_record* grt_record_Create(
                size_t internal_rbuf_len) 
{    
    grt_record* brec = malloc(sizeof(grt_record));
    brec->err = Pa_Initialize();
    if (brec->err!=paNoError) {
        brec->cb_err = grtbckErrorInit;
        return NULL;
    }
    brec->inpParams.device = Pa_GetDefaultInputDevice();
    if (brec->inpParams.device==paNoDevice) {
        brec->cb_err = grtbckErrorNoRecDevice;
        return NULL;
    }
    brec->inpParams.channelCount = NUM_CHANNELS;
    brec->inpParams.sampleFormat = paFloat32; 
    brec->inpParams.suggestedLatency = Pa_GetDeviceInfo(
                    brec->inpParams.device)->defaultLowInputLatency;
    brec->inpParams.hostApiSpecificStreamInfo = NULL;

    size_t rbufsize = internal_rbuf_len < MIN_RINGBUF_SIZE ? 
            MIN_RINGBUF_SIZE : internal_rbuf_len;

    brec->rec_cb = cbufferf_create(rbufsize);
    return brec;
}

grtbckError grt_record_StartStream(
                grt_record* brec) 
{
    brec->err = Pa_OpenStream(
                    &brec->stream,
                    &brec->inpParams,
                    NULL,
                    SAMPLE_RATE,
                    paFramesPerBufferUnspecified,
                    paNoFlag,
                    recCallback,
                    brec); 
    if (brec->err != paNoError) {
        brec->cb_err = grtbckErrorInitStream;
        return brec->cb_err;    
    }
    brec->err = Pa_StartStream(brec->stream);
    if (brec->err!=paNoError) {
        brec->cb_err= grtbckErrorOpenStream;
        return brec->cb_err;    
    }
    brec->cb_err = grtbckNoError;
    return brec->cb_err; 
}

size_t grt_record_PollNonBlocking(
                grt_record* brec, 
                float* obuffer, 
                size_t len)
{
    if (brec->cb_err!=grtbckNoError)
        return 0;
    float *samples;
    unsigned int nread;
    cbufferf_read(brec->rec_cb, len, &samples, &nread);
    cbufferf_release(brec->rec_cb, nread);
    for (size_t k=0; k<nread; k++)
        obuffer[k] = samples[k];
    return nread;
}

grtbckError grt_record_StreamStatus(
                grt_record* brec) 
{
    return brec->cb_err;
}

size_t grt_record_PollAvailableSamples(
                grt_record* brec) 
{
    return cbufferf_size(brec->rec_cb);
}

grtbckError grt_record_StopStreaming(
                grt_record* brec) 
{
    if (brec->stream) {
        brec->err = Pa_StopStream(brec->stream);
        brec->err = Pa_AbortStream(brec->stream);
        if (brec->err!=paNoError) {
            brec->cb_err = grtbckErrorCloseStream;
            return brec->cb_err; 
        }
    }
    return grtbckNoError;
}

void grt_record_Destroy(
                grt_record* brec) 
{
    if (brec) {
        brec->err = Pa_Terminate();
        cbufferf_destroy(brec->rec_cb);
        free(brec);
    }
}


