/*
 * Gretchen
 * auxiliary modules
 */
#ifndef ___GRT_AUX___
#define ___GRT_AUX___

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "hashmap.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SIGNAL CATCHING
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*
catching SIGINT (ctrl-c)
probably only posix (no windows i guess)
TODO: check and rewrite on windows!
we will abstract this part out anyway so no problem.

the thing is that calling most functions from inside a handler is unsafe!
so i just set a global variable inside the handler an do the check in the 
main function.

https://stackoverflow.com/questions/1641182/how-can-i-catch-a-ctrl-c-event-c
https://stackoverflow.com/questions/5378778/what-does-d-xopen-source-do-mean

signals are bad one should use sigaction!
http://pubs.opengroup.org/onlinepubs/9699919799/functions/sigaction.html

another way can be this:
http://zguide.zeromq.org/c:interrupt

TODO: can it fail?? How to know?? Do we need to know anyway?
*/

#define _XOPEN_SOURCE 500
#include <signal.h>
#include <unistd.h>

typedef struct {
    int grt_sigcatch_should_terminate;
} sigcatcher_t;

void grt_sigcatch_handler(int s);

void grt_sigcatch_Init();

int grt_sigcatch_ShouldTerminate();

void grt_sigcatch_Set(int i);

void grt_sigcatch_Destroy();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// FILES ENVELOPES AND RX TRASNMISSION METHODS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// binary file and related methods

unsigned long hash_djb2(unsigned char *str);

char* read_binary_file(char* filename, long* size, int* error);

void read_binary_file_size(char* filename, long* size, int* error);

void write_binary_file(char* filename, char* source, int* error);

void write_raw_file(char* filename, float* source, size_t len, int* error);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// envelope methods

#define ENVELOPE_FORMAT "%s\07%s" // delimiter is \07 ascii beep :D
#define ENVELOPE_FORMAT_DELIMITER "\07"

typedef struct {
    char *name;
    char *source;
} envelope_t;

envelope_t* envelope_create(char* name, char* source);

void envelope_destroy(envelope_t* env);

void envelope_pack(envelope_t* envelope, char** arg);

void envelope_unpack(char* envelope, envelope_t** arg);

void envelope_print(envelope_t* env);

void envelope_writeout(envelope_t* env, char* path, int* error);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// rx frame handling: chunks, transmits

typedef struct {
    unsigned int num;
    char* data;
    size_t len;
} chunk_t;

typedef struct {
    unsigned long hash;
    unsigned int max;
    chunk_t* chunks;
} transmit_t;

transmit_t* transmit_create(unsigned long hash, unsigned int max);

void transmit_destroy(transmit_t* transm);

void transmit_add(transmit_t* transm, unsigned int num, char* buffer, size_t buffer_len);

bool transmit_is_complete(transmit_t* transm);

void transmit_concatenate(transmit_t* transm, char** arg);

void transmit_print(transmit_t* transm);

void transmit_get_envelope(transmit_t* transm, envelope_t** arg);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// rx frame handler

#define RXMAP_KEY_LEN 64
#define RXMAP_KEY_FORMAT "%lu"

typedef struct {
    map_t* transmits;
} rxhandler_t;

rxhandler_t* rxhandler_create();

void rxhandler_destroy(rxhandler_t* rxm);

void rxhandler_add(rxhandler_t* rxm, unsigned long hash, unsigned int num, unsigned int max, char* buffer, size_t buffer_len);

void rxhandler_remove(rxhandler_t* rxm, unsigned long hash);

void rxhandler_get(rxhandler_t* rxm, unsigned long hash, transmit_t** arg);

void rxhandler_reap(rxhandler_t* rxm, transmit_t** ripe);

typedef void list_cb_t(transmit_t* transm, void* user);

void rxhandler_list(rxhandler_t* rxm, list_cb_t* callback, void* user);

#endif
