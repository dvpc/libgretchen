#include "gretchen.h"

static void _modemrx_callback(
                unsigned long hash,
                unsigned int frame_num,
                unsigned int frame_nummax,
                size_t buffer_len,
                uint8_t *buffer,
                void *user)
{
    gretchenRX_t* rx = (gretchenRX_t*)user;
    if (!rx)
        return ;
    // add the chunk anyway
    rxhandler_add(rx->rxhandler,
                hash, 
                frame_num, 
                frame_nummax, 
                buffer, 
                buffer_len);
    // if complete (ripe) transmits exist
    transmit_t* ripe;
    rxhandler_reap(rx->rxhandler, &ripe);
    if (!ripe)
        return ;
    // get the envelope of the transmit and call outside
    envelope_t* env;
    transmit_get_envelope(ripe, &env); 
    // FIXME what if env fails???
    //
    if (rx->callback)
        rx->callback(
                env->name, 
                env->source, 
                strlen(env->source), 
                rx->callbackuser);
    envelope_destroy(env);
    // and remove it afterwards
    rxhandler_remove(rx->rxhandler, ripe->hash);
}

static void _modemrx_progress_callback(
                unsigned long hash, 
                unsigned int frame_num, 
                unsigned int frame_nummax, 
                int payload_valid,
                void *user)
{
    gretchenRX_t* rx = (gretchenRX_t*)user;
    if (!rx)
        return ;
    if (rx->prog_callback)
        rx->prog_callback(
                hash, 
                frame_num, 
                frame_nummax,
                payload_valid, 
                rx->callbackuser);
}

gretchenRX_t* gretchenRX_create(grtModemOpt_t* opt, int internal_bufsize)
{
    gretchenRX_t* rx = malloc(sizeof(gretchenRX_t));
    rx->modem_rx = grtModemRX_create(opt, internal_bufsize);
    rx->modem_rx->emit_callback = _modemrx_callback;
    rx->modem_rx->emit_progress_callback = _modemrx_progress_callback;
    rx->modem_rx->emit_callback_userdata = rx;
    rx->rxhandler = rxhandler_create();
    rx->callback = NULL;
    rx->callbackuser = NULL;
    return rx;
}

void gretchenRX_destroy(gretchenRX_t* rx)
{
    grtModemRX_destroy(rx->modem_rx);
    rxhandler_destroy(rx->rxhandler);
    free(rx);
}

void gretchenRX_push_le16f(gretchenRX_t* rx, float* buffer, size_t len, int* error)
{
    *error = 0;
    size_t consumed;
    if (len==0) {
        grtModemRX_flush(rx->modem_rx);
        consumed = grtModemRX_consume(rx->modem_rx, buffer, 0);
    } else {
        consumed = grtModemRX_consume(rx->modem_rx, buffer, len);
    }
    if (consumed!=len) {
        *error = -1;
        return ;
    }
}





