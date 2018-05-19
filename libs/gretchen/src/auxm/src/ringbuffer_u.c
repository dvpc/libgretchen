#include "internal/ringbuffer_u.h"

rbufu_t* rbufuCreate(size_t len) {
    rbufu_t *cb = malloc(sizeof(rbufu_t));
	if (!cb)
        goto err;
    cb->maxlen = len;
    cb->buffer = malloc(len*sizeof(RBUF_U_TYPE)+4);
	if (!cb->buffer)
        goto err;
    cb->head = cb->buffer;
    cb->tail = cb->buffer;
    cb->count = 0;
    cb->haslockpop = 0;
    cb->haslockpush = 0;
    return cb;
	err:
		if (cb)
			rbufuDestroy(cb);
		return NULL;
}

void rbufuDestroy(rbufu_t *cb) {
    free(cb->buffer);
    free(cb);
}

RBUF_U_TYPE* rbufuNext(const rbufu_t* cb, RBUF_U_TYPE* ptr,  size_t len) {
    RBUF_U_TYPE *next = ptr + len;
    RBUF_U_TYPE *wrap = cb->buffer+cb->maxlen;
    if (next > wrap)
        next -= cb->maxlen; 
    return next;
}

size_t rbufuAvailable(const rbufu_t* cb) {
    if (cb->head == cb->tail) {
        if (cb->count == 0)
            return cb->maxlen; // is empty
        else
            return 0;          // is full
    }
    else if (cb->head < cb->tail) {
        size_t head = cb->head - cb->buffer;
        size_t tail = cb->tail - cb->buffer;
        return tail - head;       
    } 
    else if (cb->head > cb->tail) {
        RBUF_U_TYPE *wrap = cb->buffer + cb->maxlen;
        size_t untlwrap = wrap - cb->head;
        size_t untltail = cb->tail - cb->buffer;
        return untlwrap + untltail;
    }
    return 0; 
}

rbufError rbufuPush(rbufu_t* cb, const RBUF_U_TYPE* ibuf, size_t len) {
    if (cb->haslockpush==1)
        return rbufErrorBusy;
    size_t avail = rbufuAvailable(cb);
    if (avail==0) {
        return rbufErrorFull;
    }
    if (len > avail) {
        return rbufErrorRequestTooLarge;
    }
    cb->haslockpush = 1;
    if (cb->head <= cb->tail) { 
        memmove(cb->head, ibuf, len*sizeof(RBUF_U_TYPE));    
    } 
    else 
    {
        size_t untlwrap = cb->buffer+cb->maxlen - cb->head;
        if (untlwrap > len) {
            memmove(cb->head, ibuf, len*sizeof(RBUF_U_TYPE));
        }
        else {
            memmove(cb->head, ibuf, untlwrap*sizeof(RBUF_U_TYPE));
            memmove(cb->buffer, ibuf+untlwrap, (len-untlwrap)*sizeof(RBUF_U_TYPE));
        }
    }
    cb->head = rbufuNext(cb, cb->head, len); 
    cb->count += len;
    cb->haslockpush = 0;
    return rbufNoError;
}


rbufError rbufuPop(rbufu_t* cb, RBUF_U_TYPE* obuf, size_t len) {
    if (cb->haslockpop==1)
        return rbufErrorBusy;
    if (cb->count==0) {
        return rbufErrorEmpty;
    }     
    if (len > cb->count) {
        return rbufErrorRequestTooLarge;   
    }
    cb->haslockpop = 1;
    if (cb->head > cb->tail) {
        memmove(obuf, cb->tail, len*sizeof(RBUF_U_TYPE)); 
    }
    else
    {
        size_t untlwrap = cb->buffer+cb->maxlen - cb->tail;
        if (untlwrap > len) {
            memmove(obuf, cb->tail, len*sizeof(RBUF_U_TYPE));
        }
        else {
            memmove(obuf, cb->tail, untlwrap*sizeof(RBUF_U_TYPE));
            size_t left = len - untlwrap;
            memmove(obuf+untlwrap, cb->buffer, left*sizeof(RBUF_U_TYPE));
        }
    }
    cb->tail = rbufuNext(cb, cb->tail, len);
    cb->count -= len;
    cb->haslockpop = 0;
    return rbufNoError;
}

void rbufuPrintBuffer(const rbufu_t* cb) {
    printf("maxlen:%lu; usedEntries:%lu; typesize:%lu\n",
        cb->maxlen, cb->count, sizeof(RBUF_U_TYPE));
    for (size_t i=0; i<cb->maxlen; i++) {
        printf("pos:%lu value:%i \n", i, *(cb->buffer + i));
    }
    printf("\n");
}

void rbufuReset(rbufu_t* cb) {
    cb->head = cb->buffer;
    cb->tail = cb->buffer;
    cb->count = 0;
    cb->haslockpop = 0;
    cb->haslockpush = 0;
}
