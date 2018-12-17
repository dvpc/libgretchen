#include "gretchen.h"

static void _modemrx_callback(
                uint16_t hash,
                uint16_t frame_num,
                uint16_t frame_nummax,
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
    transmit_create_envelope(ripe, &env); 
    // FIXME 
    // what if env fails???
    if (rx->callback)
        rx->callback(
                env->name, 
                env->source, 
                strlen((char*)env->source), 
                rx->callbackuser);
    envelope_destroy(env);
    // and remove it afterwards
    rxhandler_remove(rx->rxhandler, ripe->hash);
}

static void _modemrx_progress_callback(
                uint16_t hash, 
                uint16_t frame_num, 
                uint16_t frame_nummax, 
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

gretchenRX_t* gretchenRX_create(grtModemOpt_t* opt, size_t internal_bufsize)
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
    if (!rx)
        return;
    grtModemRX_destroy(rx->modem_rx);
    rxhandler_destroy(rx->rxhandler);
    free(rx);
}

void gretchenRX_push_le16f(gretchenRX_t* rx, float* buffer, size_t len, int8_t* error)
{
    *error = 0;
    size_t consumed;
    if (len==0) {
        grtModemRX_enable_flush(rx->modem_rx);
        consumed = grtModemRX_consume(rx->modem_rx, buffer, 0);
    } else {
        consumed = grtModemRX_consume(rx->modem_rx, buffer, len);
    }
    if (consumed!=len) {
        *error = -1;
        return ;
    }
}





void gretchenRX_set_filecomplete_cb(gretchenRX_t* rx, gretchenRX_filecomplete_callback* cb)
{
    rx->callback = cb;
}

void gretchenRX_set_progress_cb(gretchenRX_t* rx, gretchenRX_progress_callback* cb)
{
    rx->prog_callback = cb;
}

void gretchenRX_set_callback_userdata(gretchenRX_t* rx, void* user)
{
    rx->callbackuser = user;
}

void gretchenRX_set_debug_cb(gretchenRX_t* rx, grtModemRX_emit_debug_callback* cb)
{
    rx->modem_rx->emit_debug_callback = cb;
}
