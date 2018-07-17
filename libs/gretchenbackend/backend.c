#include "gretchen.backend.h"

static size_t _buffer_available();
static int _play_callback();
static int _record_callback();

grtBackend_t* grtBackend_create(size_t internalbufsize, bool is_tx, bool is_rx_multichannel)
{
    grtBackend_t* back = malloc(sizeof(grtBackend_t));
    if (!back)
        goto error;
    back->is_tx = is_tx;
    back->err = Pa_Initialize();
    if (back->err != paNoError)
        goto error;
    back->strParams.device = is_tx ?
            Pa_GetDefaultOutputDevice() : Pa_GetDefaultInputDevice();
    if (back->strParams.device==paNoDevice)
        goto error; 
    if (!is_tx && is_rx_multichannel) {
        // FIXME
        // i get on my arch box the value of maxInputChannels == 128!!!
        // that is a bit too much and i guess my box has not 128 microphones...
        // const PaDeviceInfo* info = Pa_GetDeviceInfo(back->strParams.device);
        // back->strParams.channelCount = info->maxInputChannels;
        back->strParams.channelCount = 2;
    } else {
        back->strParams.channelCount = 1;
    }
    back->strParams.sampleFormat = paFloat32;
    back->strParams.suggestedLatency = is_tx ? 
            Pa_GetDeviceInfo(back->strParams.device)->defaultLowOutputLatency : 
            Pa_GetDeviceInfo(back->strParams.device)->defaultLowInputLatency;
    back->strParams.hostApiSpecificStreamInfo = NULL;
    size_t bufsize = internalbufsize * back->strParams.channelCount;
    back->samplebuffer = cbufferf_create(bufsize);
    return back;
    error:
        if (back)
            grtBackend_destroy(back);
        return NULL;
}

void grtBackend_destroy(grtBackend_t* back)
{
    if (back) {
        Pa_Terminate();
        if (back->samplebuffer)
            cbufferf_destroy(back->samplebuffer);
    }
}

void grtBackend_startstream(grtBackend_t* back, int* error)
{
    *error = 0;
    if (back->is_tx) {
        back->err = Pa_OpenStream(
                        &back->stream, NULL, &back->strParams,
                        44100, paFramesPerBufferUnspecified,
                        paDitherOff | paClipOff, _play_callback,
                        back);
    } else {
        back->err = Pa_OpenStream(
                        &back->stream, &back->strParams, NULL,
                        44100, paFramesPerBufferUnspecified,
                        paDitherOff | paClipOff, _record_callback,
                        back); 
    }
    if (back->err != paNoError)
        *error = -1;
    back->err = Pa_StartStream(back->stream);
    if (back->err != paNoError)
        *error = -2;
}

bool grtBackend_isstreamactive(grtBackend_t* back)
{
    return Pa_IsStreamActive(back->stream);
}

void grtBackend_stopstream(grtBackend_t* back, int* error)
{
    *error = 0;
    back->err = Pa_StopStream(back->stream);
    back->err = Pa_CloseStream(back->stream);
    if (back->err != paNoError)
        *error = -2;
}

size_t grtBackend_push_available(grtBackend_t* back)
{
    return _buffer_available(back);
}

size_t grtBackend_push(grtBackend_t* back, float* buffer, size_t len)
{
    size_t avail = _buffer_available(back);
    if (avail==0)
        return 0;
    size_t num = avail < len ? avail : len;
    cbufferf_write(back->samplebuffer, buffer, num);
    return num;
}

void grtBackend_poll(grtBackend_t* back, size_t ask, float** buffer, size_t* len)
{
    *len = 0;
    *buffer = NULL;
    if (back->err!=paNoError)
        return ;
    unsigned int nread;
    cbufferf_read(back->samplebuffer, ask, buffer, &nread);
    cbufferf_release(back->samplebuffer, nread);
    *len = nread;
}

static size_t _buffer_available(grtBackend_t* back)
{
    return cbufferf_max_size(back->samplebuffer) - 
           cbufferf_size(back->samplebuffer);
}

static int _play_callback(
                const void *inbuf, 
                const void *outbuf,
                unsigned long frmsPerBuf, 
                const PaStreamCallbackTimeInfo* timeInfo,
                PaStreamCallbackFlags statFlags,
                void *user)
{
    (void) inbuf;
    (void) statFlags;
    (void) timeInfo;
    grtBackend_t* back = (grtBackend_t*)user;
    float *outp = (float*)outbuf;
    float *samples;
    unsigned int nread;
    cbufferf_read(back->samplebuffer, frmsPerBuf, &samples, &nread);
    cbufferf_release(back->samplebuffer, nread);
    for (size_t k=0; k<nread; k++)
        outp[k] = samples[k];
    return paContinue;
} 

static int _record_callback(
                const void *inbuf, 
                const void *outbuf,
                unsigned long frmsPerBuf, 
                const PaStreamCallbackTimeInfo* timeInfo,
                PaStreamCallbackFlags statFlags,
                void *user)
{
    (void) outbuf;
    (void) statFlags;
    (void) timeInfo;
    grtBackend_t* back = (grtBackend_t*)user;
    const float *inp = (const float*)inbuf;
    size_t avail = _buffer_available(back);

    unsigned int channels = back->strParams.channelCount;
    if (avail < frmsPerBuf*channels) {
        back->err = paInputOverflowed;
        return paComplete;
    }
    void* tmprecbuf = alloca(frmsPerBuf*sizeof(float)*channels);
    float *wptr = (float*)tmprecbuf;
    if (inp==NULL) {
        for (size_t k=0; k<frmsPerBuf; k++) {
            *wptr++ = 0.0f;
            if (channels==2) *wptr++ = 0.0f;
        }
    } else {
        for (size_t k=0; k<frmsPerBuf; k++) {
            *wptr++ = *inp++;
            if (channels==2) *wptr++ = *inp++;
        }
    }
    cbufferf_write(back->samplebuffer, tmprecbuf, frmsPerBuf*channels); 
    return paContinue;
}




