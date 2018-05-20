#include "gretchen.h"

static void _modemtx_callback(
                size_t buffer_len, 
                float *buffer, 
                void *user)
{
    gretchenTX_t* tx = (gretchenTX_t*) user;
    size_t wantlen = tx->samples_len+buffer_len;
    tx->samples = realloc(tx->samples, wantlen*sizeof(float));
    if (!tx->samples)
        return ;
    memcpy(tx->samples+tx->samples_len, buffer, buffer_len*sizeof(float));
    tx->samples_len = wantlen;

}

gretchenTX_t* gretchenTX_create(grtModemOpt_t* opt, int internalbufsize)
{
    gretchenTX_t* tx = malloc(sizeof(gretchenTX_t));
    tx->packed_env = NULL;
    tx->samples = NULL;
    tx->samples_len = 0;
    tx->modem_tx = grtModemTX_create(opt, internalbufsize);
    tx->modem_tx->emit_callback = _modemtx_callback;
    tx->modem_tx->emit_callback_userdata = tx;
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

void gretchenTX_inspect(gretchenTX_t* tx, char* filename, int* error, gretchenTX_inspect_t** info)
{
    *info = malloc(sizeof(gretchenTX_inspect_t));
    if (!*info)
        return ;
    long filesize;
    read_binary_file_size(filename, &filesize, *&error);
    if (*error!=0)
        return ;
    if (filesize > MAX_SUGGESTED_FILESIZE)
        (*info)->is_toolarge = 1;
    else
        (*info)->is_toolarge = 0;
    (*info)->filesize_bytes = filesize;
    // a simple pessimistic guess of how many samples are needed
    // pessimistic == counting until next complete frame!! --> overshoot
    grtModemOpt_t opt = tx->modem_tx->opt;
    unsigned int num = ceil(filesize / opt.frameopt->frame_len)+1;
    unsigned int bps = opt.frameopt->_bits_per_symbol;
    unsigned int bits_frame = tx->modem_tx->framelen_symbols * bps; 
    unsigned int bits_flush = tx->modem_tx->mod->flushlen * bps;
    long bits_est = num * (bits_frame+bits_flush);
    long samples_est = (bits_est/bps)*opt.modopt->samples_per_symbol;
    // put estimates in struc
    (*info)->est_encodedsize_samples = samples_est;
    (*info)->est_transfer_sec = samples_est / 44100; // TODO variable samplerate
}

void gretchenTX_prepare(gretchenTX_t* tx, char* filename, int* error)
{
    // load the file
    long filesize;
    char *source = read_binary_file(filename, &filesize, *&error);
    if (*error!=0)
        return ;

    // create and pack envelope 
    envelope_t* env = envelope_create(filename, source);
    char *envstr;
    envelope_pack(env, &envstr); 
    tx->packed_env = envstr;

    // set modem header info
    size_t packlen = strlen(tx->packed_env);
    unsigned long hash = hash_djb2((unsigned char*) env->source);    
    grtModemTX_setheaderinfo(tx->modem_tx, hash, packlen);

    // modulate packed env
    // TODO think of a good way to restrict the buffer / chunk sizes
    // it has to be smaller than the internal buffer of the modem!!!
    //
    size_t chunk_want = 1 << 10; 
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

void gretchenTX_get(gretchenTX_t* tx, float** samplebuffer, size_t* len)
{
    *len = 0;
    *samplebuffer = NULL;
    if (!tx->samples)
        return ;
    *len = tx->samples_len;
    *samplebuffer = malloc(sizeof(float)*tx->samples_len);
    memcpy(*samplebuffer, tx->samples, tx->samples_len*sizeof(float));
}



