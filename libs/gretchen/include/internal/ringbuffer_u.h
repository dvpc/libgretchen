/* 
 * Gretchen
 * Simple ring buffer with memcpy
 * not thread safe!
 * ugly and simple :D
 *
 * NOTE!!!!!!! about memcpy
 * Some notes on memcpy
 * http://brnz.org/hbr/?p=1094
 */

#ifndef ___RINGBUFFER_U___
#define ___RINGBUFFER_U___

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum rbufError {
    rbufNoError = 0,
    rbufErrorEmpty = -1,
    rbufErrorFull = -2,
    rbufErrorRequestTooLarge = -3,  
    rbufErrorBusy = -4
} rbufError;

typedef uint8_t RBUF_U_TYPE;

typedef struct {
    RBUF_U_TYPE *buffer;
    RBUF_U_TYPE *head;
    RBUF_U_TYPE *tail;
    size_t maxlen;
    size_t count;
    int haslockpop;
    int haslockpush;
} rbufu_t;

/*
 * Returns a pointer to created ringbuffer of size len. 
 * NULL on error.
 */
rbufu_t* rbufuCreate(size_t len);

/*
 * Destroys the ringbuffer.
 */
void rbufuDestroy(rbufu_t* cb);

/*
 * Returns the number of available elements to store.
 * AKA free space.
 */
size_t rbufuAvailable(const rbufu_t* cb);

/*
 * Passes a buffer of size len to be pushed into the ringbuffer.
 * Returns `rbufErrorRequestTooLarge` error if buffer is greater 
 * than the internal available space.
 * If the buffer is full it returns a `rbufErrorFull` error. 
 * If all went well a `rbufNoError` is returned.
 */
rbufError rbufuPush(rbufu_t* cb, const RBUF_U_TYPE* ibuf, size_t len);

/*
 * Fills a passed buffer of len with internal data. 
 * If more data is reuqested than available it returns 
 * a `rbufErrorRequestTooLarge` error. 
 * If the buffer is empty a `rbufErrorEmpty` error is returned.
 * If all went well a `rbufNoError` is returned.
 */
rbufError rbufuPop(rbufu_t* cb, RBUF_U_TYPE* obuf, size_t len);

/*
 * Prints the whole internal buffer state to stdout.
 * Idk, but maybe replace with fprintf to strerr?? 
 */
void rbufuPrintBuffer(const rbufu_t* cb);

/*
 * Resets the internal state of the ringbuffer
 */
void rbufuReset(rbufu_t* cb);

#endif


