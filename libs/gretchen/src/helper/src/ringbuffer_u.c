#include "gretchen.internal.h"

rbufu_t* rbufuCreate(size_t len) {
    rbufu_t *cb = malloc(sizeof(rbufu_t));
	if (!cb)
        goto err;
    cb->maxlen = len;
    cb->buffer = calloc(len+2,sizeof(RBUF_TYPE));
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

RBUF_TYPE* rbufuNext(const rbufu_t* cb, RBUF_TYPE* ptr,  size_t len) {
    RBUF_TYPE *next = ptr + len;
    RBUF_TYPE *wrap = cb->buffer+cb->maxlen;
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
        RBUF_TYPE *wrap = cb->buffer + cb->maxlen;
        size_t untlwrap = wrap - cb->head;
        size_t untltail = cb->tail - cb->buffer;
        return untlwrap + untltail;
    }
    return 0; 
}

int8_t rbufuPush(rbufu_t* cb, const RBUF_TYPE* ibuf, size_t len) {
    if (cb->haslockpush==1)
        return RBUF_BUSY;
    size_t avail = rbufuAvailable(cb);
    if (avail==0) {
        return RBUF_FULL;
    }
    if (len > avail) {
        return RBUF_TOOLARGE;
    }
    cb->haslockpush = 1;
    if (cb->head <= cb->tail) { 
        memmove(cb->head, ibuf, len*sizeof(RBUF_TYPE));    
    } 
    else 
    {
        size_t untlwrap = cb->buffer+cb->maxlen - cb->head;
        if (untlwrap > len) {
            memmove(cb->head, ibuf, len*sizeof(RBUF_TYPE));
        }
        else {
            memmove(cb->head, ibuf, untlwrap*sizeof(RBUF_TYPE));
            memmove(cb->buffer, ibuf+untlwrap, (len-untlwrap)*sizeof(RBUF_TYPE));
        }
    }
    cb->head = rbufuNext(cb, cb->head, len); 
    cb->count += len;
    cb->haslockpush = 0;
    return RBUF_OK;
}

int8_t rbufuPop(rbufu_t* cb, RBUF_TYPE* obuf, size_t len) {
    if (cb->haslockpop==1)
        return RBUF_BUSY;
    if (cb->count==0) {
        return RBUF_EMPTY;
    }     
    if (len > cb->count) {
        return RBUF_TOOLARGE;
    }
    cb->haslockpop = 1;
    if (cb->head > cb->tail) {
        memmove(obuf, cb->tail, len*sizeof(RBUF_TYPE)); 
    }
    else
    {
        size_t untlwrap = cb->buffer+cb->maxlen - cb->tail;
        if (untlwrap > len) {
            memmove(obuf, cb->tail, len*sizeof(RBUF_TYPE));
        }
        else {
            memmove(obuf, cb->tail, untlwrap*sizeof(RBUF_TYPE));
            size_t left = len - untlwrap;
            memmove(obuf+untlwrap, cb->buffer, left*sizeof(RBUF_TYPE));
        }
    }
    cb->tail = rbufuNext(cb, cb->tail, len);
    cb->count -= len;
    cb->haslockpop = 0;
    return RBUF_OK;
}

void rbufuPrintBuffer(const rbufu_t* cb) {
    printf("maxlen:%lu; usedEntries:%lu; typesize:%lu\n",
        cb->maxlen, cb->count, sizeof(RBUF_TYPE));
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
