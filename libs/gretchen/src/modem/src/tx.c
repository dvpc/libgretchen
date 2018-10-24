#include "gretchen.h"

static void _estimate_transmission(gretchenTX_t* tx, size_t filesize, gretchenTX_inspect_t** info)
{
    // better (a bit too optimistic) approach
    grtModemOpt_t opt = tx->modem_tx->opt;
    size_t symb_header = framegen_estimate_num_symbols(tx->modem_tx, 0);
    size_t symb_frame = tx->modem_tx->framelen_symbols;
    size_t symb_payload = symb_frame - symb_header;
    size_t symb_flush = tx->modem_tx->mod->flushlen;
    double num_payloads = (double)filesize / (double)opt.frameopt->payload_len;
    size_t symb_total = ceil(num_payloads)*(double)symb_header +
    num_payloads*(double)symb_payload + ceil(num_payloads)*symb_flush;
    size_t samples_est = symb_total*opt.modopt->samples_per_symbol;
    // put estimates in struc
    *info = malloc(sizeof(gretchenTX_inspect_t));
    if (!*info)
        return ;
    (*info)->filesize_bytes = filesize;
    (*info)->est_encodedsize_samples = samples_est;
    (*info)->est_transfer_sec = samples_est / tx->modem_tx->opt.samplerate;
}

static void _prepare_for_transmission(gretchenTX_t* tx, uint8_t* filename, uint8_t* source)
{
    // create and pack envelope
    envelope_t* env = envelope_create(filename, source);
    uint8_t *envstr;
    envelope_pack(env, &envstr);
    tx->packed_env = envstr;
    
    // set modem header info
    size_t packlen = strlen((char*)tx->packed_env);
    uint64_t hash = hash_djb2(env->source);
    grtModemTX_setheaderinfo(tx->modem_tx, hash, packlen);
    
    // modulate packed env
    size_t chunk_want = tx->modem_tx->internal_bufsize >> 2;
    size_t chunk;
    size_t k=0;
    while(true) {
        if (k+chunk_want > packlen) {
            chunk = packlen-k;
            grtModemTX_enable_flush(tx->modem_tx);
            grtModemTX_consume(tx->modem_tx, tx->packed_env+k, chunk);
            break;
        } else {
            chunk = chunk_want;
            grtModemTX_consume(tx->modem_tx, tx->packed_env+k, chunk);
        }
        k += chunk;
    }
    envelope_destroy(env);
    free(source);
}

static void _modemtx_callback(size_t buffer_len, float *buffer, void *user)
{
    gretchenTX_t* tx = (gretchenTX_t*) user;
    size_t wantlen = tx->samples_len+buffer_len;
    tx->samples = realloc(tx->samples, wantlen*sizeof(float));
    if (!tx->samples)
        return ;
    memcpy(tx->samples+tx->samples_len, buffer, buffer_len*sizeof(float));
    tx->samples_len = wantlen;
    if (tx->prog_callback)
        tx->prog_callback(wantlen, tx->callbackuser);
}

gretchenTX_t* gretchenTX_create(grtModemOpt_t* opt, size_t internalbufsize)
{
    gretchenTX_t* tx = malloc(sizeof(gretchenTX_t));
    tx->packed_env = NULL;
    tx->samples = NULL;
    tx->samples_len = 0;
    tx->modem_tx = grtModemTX_create(opt, internalbufsize);
    tx->modem_tx->emit_callback = _modemtx_callback;
    tx->modem_tx->emit_callback_userdata = tx;
    tx->prog_callback = NULL;
    tx->callbackuser = NULL;
    return tx;
}

void gretchenTX_destroy(gretchenTX_t* tx)
{
    if (tx->packed_env)
        free(tx->packed_env);
    if (tx->samples)
        free(tx->samples);
    grtModemTX_destroy(tx->modem_tx);
    free(tx);
}

void gretchenTX_inspect(gretchenTX_t* tx, uint8_t* filename, int8_t* error, gretchenTX_inspect_t** info)
{
    int64_t filesize;
    optain_binaryfile_size(filename, &filesize, *&error);
    if (*error!=0)
        return ;
    _estimate_transmission(tx, filesize, &(*info));
}

void gretchenTX_inspect_buffer(gretchenTX_t* tx, uint8_t* buffer, gretchenTX_inspect_t** info)
{
    if (!*buffer)
        return ;
    int64_t filesize = strlen((char*) buffer);
    _estimate_transmission(tx, filesize, &(*info));
}

void gretchenTX_prepare(gretchenTX_t* tx, uint8_t* filename, int8_t* error)
{
    // load the file
    int64_t filesize;
    uint8_t *source = read_binaryfile(filename, &filesize, *&error);
    if (*error!=0)
        return ;
    _prepare_for_transmission(tx, filename, source);
}

void gretchenTX_prepare_buffer(gretchenTX_t* tx, uint8_t* buffer)
{
    uint8_t* filename = malloc(sizeof(uint8_t)*6);
    strcpy((char*) filename, "none\0");
    _prepare_for_transmission(tx, filename, buffer);
    free(filename);
}

void gretchenTX_get(gretchenTX_t* tx, float** samplebuffer, size_t* len)
{
    *len = 0;
    *samplebuffer = NULL;
    if (!tx->samples)
        return ;
    // append and prepend .25 seconds of silence
    // to avoid clicks when playing the sample back
    // TODO has to be updated when having variable samplerate...
    *len = tx->samples_len+22050;
    *samplebuffer = calloc(sizeof(float), tx->samples_len+22050);
    memcpy(*samplebuffer+11025, tx->samples, tx->samples_len*sizeof(float));
}

void gretchenTX_set_progress_cb(gretchenTX_t* tx, gretchenTX_progress_callback* cb)
{
    tx->prog_callback = cb;
}

void gretchenTX_set_callback_userdata(gretchenTX_t* rx, void* user){
    rx->callbackuser = user;
}

