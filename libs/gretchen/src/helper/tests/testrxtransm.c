#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "gretchen.internal.h"


static void print_transm(transmit_t* t, void* user)
{
    (void) user;
    transmit_print(t);
}

static void debug_print(rxhandler_t* rxm) {
    printf("----------------\n");
    rxhandler_list(rxm, print_transm, NULL);
    printf("----------------\n");
}

int main(int argc, char **argv) {
    (void) argv;
    (void) argc;

    // dummy data
    uint16_t hash;
    size_t buflen = 50;
    char buffer[buflen];
    memset(buffer, '\0', 50);


    printf("rx handler test\n");
    rxhandler_t* rxm = rxhandler_create();


    // 1st transmit
    strcpy(buffer, "somedatawhatever\0");
    hash = (uint16_t) hash_djb2((uint8_t*)buffer);
    // 1st transmit some chunks
    strcpy(buffer, "anotherfile.txt;;__a0123456789\0");
    rxhandler_add(rxm, hash, 0, 3, (uint8_t*)buffer, buflen);
    strcpy(buffer, "__c0123456789\0");
    rxhandler_add(rxm, hash, 2, 3, (uint8_t*)buffer, buflen);
    strcpy(buffer, "__d0123456789\0");
    rxhandler_add(rxm, hash, 3, 3, (uint8_t*)buffer, buflen);

    // 2nd transmit
    strcpy(buffer, "somethingelseisthishere\0");
    uint16_t hash2 = hash_djb2((uint8_t*)buffer);
    // 2nd transmit some chunks
    strcpy(buffer, "alltest.txt;;__aabcdefghijklmnopqrstuvwx\0");
    rxhandler_add(rxm, hash2, 0, 3, (uint8_t*)buffer, buflen);
    /*strcpy(buffer, "babcdefghijklmnopqrstuvwx\0");*/
    /*rxhandler_add(rxm, hash2, 1, 3, (uint8_t*)buffer, buflen);*/
    /*strcpy(buffer, "cabcdefghijklmnopqrstuvwx\0");*/
    /*rxhandler_add(rxm, hash2, 2, 3, (uint8_t*)buffer, buflen);*/

    printf("\n>   after one incomplete and one complete insertions\n");
    debug_print(rxm);


    strcpy(buffer, "a0123456789\0");
    rxhandler_add(rxm, hash, 0, 3, (uint8_t*)buffer, buflen);

    printf("\n>   after inserting the next wrong buffer\n");
    debug_print(rxm);
    

    strcpy(buffer, "__b0123456789\0");
    rxhandler_add(rxm, hash, 1, 3, (uint8_t*)buffer, buflen);
    
    printf("\n>   after inserting the next right buffer\n");
    debug_print(rxm);





    transmit_t* t1 = NULL;
    rxhandler_get(rxm, hash, &t1);
    printf("\n>   the 1st hash is %hu", t1->hash);
    printf("\n>   getting the envelope of %p\n", t1);
    envelope_t* env;
    transmit_create_envelope(t1, &env);
    envelope_print(env);



    int8_t error;
    envelope_writeout(env, (uint8_t*)"test/0", &error);
    if (error==0)
        printf("File written: %s error:%i\n", env->name, error);
    else
        printf("File NOT written: %s error:%i\n", env->name, error);
    envelope_destroy(env);

    rxhandler_remove(rxm, hash);
    printf("\n>   after removing 1 st hash\n");
    debug_print(rxm);


    printf("\n>   a reentry of the removed hash should be ignored\n");
    // FIXME
    // test this on other platforms...

    printf("\n>   after trying to add 1st hash again\n");
    debug_print(rxm);

    strcpy(buffer, "__d0123456789\0");
    rxhandler_add(rxm, hash, 0, 3, (uint8_t*)buffer, buflen);


    printf("\n>   done\n");
    rxhandler_destroy(rxm);

    return 0;
}



