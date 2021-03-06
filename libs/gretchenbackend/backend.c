#include "gretchen.backend.h"

static size_t _buffer_available();
static int _play_callback();
static int _record_callback();


// since its impossible to directly query portaudio for the real  
// number of input channels we do it manually here
// by simply checking if a channel only contains zeros 
// which indicates a virtual channel.
static int _estimator_num_input_channels_callback();
typedef struct {
    uint32_t num_channels;
    bool chlimit_reached;
} _estimator_num_input_channels_data_t;

void grtBackend_estimate_inputdecive_numchannels(PaDeviceIndex device, uint32_t* result_num_channel, int8_t* error, uint32_t samplerate)
{
    *error = 0;
    *result_num_channel = 0;
    PaStreamParameters strParams;
    strParams.device = device;
    if (strParams.device==paNoDevice)
        return ;
    strParams.sampleFormat = paFloat32 | paNonInterleaved;
    strParams.suggestedLatency = 
            Pa_GetDeviceInfo(strParams.device)->defaultLowInputLatency;
    strParams.hostApiSpecificStreamInfo = NULL;
    _estimator_num_input_channels_data_t* data = 
            malloc(sizeof(_estimator_num_input_channels_data_t));
    data->num_channels = 1;
    data->chlimit_reached = false;
    PaError err = paNoError;
    while(true) {
        strParams.channelCount = data->num_channels;
        PaStream* stream; 
        err = Pa_OpenStream(&stream, &strParams, NULL,
                            samplerate, paFramesPerBufferUnspecified,
                            paDitherOff | paClipOff, 
                            _estimator_num_input_channels_callback,
                            data); 
        if (err!=paNoError) {
            goto while_exit;
        }
        err = Pa_StartStream(stream);
        if (err!=paNoError) {
            goto while_exit;
        }
        Pa_Sleep(1);
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        if (data->chlimit_reached) {
            goto while_exit;
        }
        data->num_channels ++;
    }

while_exit:
    *error = err;
    *result_num_channel = data->num_channels-1;

    free(data);
}


grtBackend_t* grtBackend_create(size_t internalbufsize, bool is_tx, uint32_t samplerate)
{
    grtBackend_t* back = malloc(sizeof(grtBackend_t));
    if (!back)
        goto error_create;
    back->is_tx = is_tx;
    back->samplerate = samplerate;
    back->err = Pa_Initialize();
    if (back->err != paNoError)
        goto error_create;
    back->strParams.sampleFormat = paFloat32;
    back->strParams.hostApiSpecificStreamInfo = NULL;
    if (is_tx) {
        back->strParams.device = Pa_GetDefaultOutputDevice();
        if (back->strParams.device==paNoDevice)
            goto error_create; 
        back->strParams.channelCount = 1;
        back->strParams.suggestedLatency = 
            Pa_GetDeviceInfo(back->strParams.device)->defaultLowOutputLatency;
    } else {
        back->strParams.device = Pa_GetDefaultInputDevice();
        if (back->strParams.device==paNoDevice)
            goto error_create; 
        int8_t error;
        uint32_t num_input_channels;
        grtBackend_estimate_inputdecive_numchannels(
                        back->strParams.device,
                        &num_input_channels, 
                        &error,
                        samplerate);
        if (num_input_channels==0)
            goto error_create;
        back->strParams.channelCount = num_input_channels;
        back->strParams.suggestedLatency = 
                Pa_GetDeviceInfo(back->strParams.device)->defaultLowInputLatency;
    }
    size_t bufsize = internalbufsize * back->strParams.channelCount;
    back->samplebuffer = cbufferf_create(bufsize);
    return back;
error_create:
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

void grtBackend_startstream(grtBackend_t* back, int8_t* error)
{
    *error = 0;
    if (back->is_tx) {
        back->err = Pa_OpenStream(
                        &back->stream, NULL, &back->strParams,
                        back->samplerate, paFramesPerBufferUnspecified,
                        paDitherOff | paClipOff, _play_callback,
                        back);
    } else {
        back->err = Pa_OpenStream(
                        &back->stream, &back->strParams, NULL,
                        back->samplerate, paFramesPerBufferUnspecified,
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

const uint8_t* grtBackend_getstatustext(grtBackend_t* back)
{
    return (uint8_t*) Pa_GetErrorText(back->err);
}

void grtBackend_stopstream(grtBackend_t* back, int8_t* error)
{
    *error = 0;
    back->err = Pa_StopStream(back->stream);
    back->err = Pa_CloseStream(back->stream);
    if (back->err != paNoError)
        *error = -3;
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
    uint32_t nread;
    cbufferf_read(back->samplebuffer, ask, buffer, &nread);
    cbufferf_release(back->samplebuffer, nread);
    *len = nread;
}

static size_t _buffer_available(grtBackend_t* back)
{
    return cbufferf_max_size(back->samplebuffer) - 
           cbufferf_size(back->samplebuffer);
}

// FIXME
// seems bullshit
// evaluate
// is it better to just force playing back a sample only using 
// ONE speaker??? to avoid possibly smaering the output??
static int _play_callback(
                const void *inbuf, 
                void *outbuf,
                uint64_t frmsPerBuf, 
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
    uint32_t nread;
    cbufferf_read(back->samplebuffer, frmsPerBuf, &samples, &nread);
    cbufferf_release(back->samplebuffer, nread);
    for (size_t k=0; k<nread; k++)
        outp[k] = samples[k];
    // FIXME testing
    // discarding all but the 1st channel 
    /*uint32_t channels = back->strParams.channelCount;*/
    /*size_t idx=0;*/
    /*for (size_t k=0; k<nread; k+=channels) {*/
        /*outp[idx++] = samples[k];*/
        /*outp[idx++] = 0.0f;*/
    /*}*/
    return paContinue;
} 

static int _record_callback(
                const void *inbuf, 
                void *outbuf,
                uint64_t frmsPerBuf, 
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
    uint32_t channels = back->strParams.channelCount;
    /*fprintf(stderr, "a:%zu -> f:%zu     ", avail, frmsPerBuf*channels);*/
    if (avail < frmsPerBuf*channels) {
        back->err = paInputOverflowed;
        return paComplete;
    }
    void* tmprecbuf = alloca(frmsPerBuf*sizeof(float)*channels);
    float *wptr = (float*)tmprecbuf;
    if (inp==NULL) {
        for (size_t k=0; k<frmsPerBuf; k++) {
            for (uint32_t j=0; j<channels; j++)
                *wptr++ = 0.0f;
        }
    } else {
        for (size_t k=0; k<frmsPerBuf; k++) {
            for (uint32_t j=0; j<channels; j++)
                *wptr++ = *inp++;
        }
    }
    cbufferf_write(back->samplebuffer, tmprecbuf, frmsPerBuf*channels); 
    return paContinue;
}

static int _estimator_num_input_channels_callback(
                const void *inbuf, 
                void *outbuf,
                uint64_t frmsPerBuf, 
                const PaStreamCallbackTimeInfo* timeInfo,
                PaStreamCallbackFlags statFlags,
                void *user)
{
    (void) outbuf;
    (void) statFlags;
    (void) timeInfo;
    _estimator_num_input_channels_data_t* data = 
            (_estimator_num_input_channels_data_t*) user;
    if (inbuf == NULL)
        return paComplete;
    else {
        size_t chlen = frmsPerBuf / data->num_channels;
        float* interlv = (float*) inbuf;
        for (uint32_t j=0; j<data->num_channels; j++) {
/*fprintf(stderr,"c%u: ", j); */
            bool all_zero = true;
            float* chp = ((float**) interlv)[j];
            for (size_t k=0; k<chlen; k++) {
                all_zero = chp[k]==0?true:false;
/*fprintf(stderr, "%f(%s)", chp[k], (chp[k]==0?"y":"n"));*/
                if (!all_zero)
                    break;
            } 
/*fprintf(stderr,"\n"); */
            if (all_zero) {
                data->chlimit_reached = true;
            }
        }
    }
    return paComplete;
}

