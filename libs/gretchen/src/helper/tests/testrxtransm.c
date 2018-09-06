#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "gretchen.internal.h"


static void print_transm(transmit_t* t, void* user)
{
    (void) user;
    transmit_print(t);
}



int main(int argc, char **argv) {
    (void) argv;
    (void) argc;

    // dummy data
    uint64_t hash;
    size_t buflen = 50;
    uint8_t buffer[buflen];
    memset(buffer, '\0', 50);


    printf("rx handler test\n");

    rxhandler_t* rxm = rxhandler_create();


    // 1st transmit
    // just get a hash...
    strcpy(buffer, "somedatawhatever\0");
    hash = hash_djb2(buffer);
    // 1st transmit some chunks
    strcpy(buffer, "anotherfile.txt\07__a0123456789\0");
    rxhandler_add(rxm, hash, 0, 3, buffer, buflen);
    strcpy(buffer, "__c0123456789\0");
    rxhandler_add(rxm, hash, 2, 3, buffer, buflen);
    strcpy(buffer, "__d0123456789\0");
    rxhandler_add(rxm, hash, 3, 3, buffer, buflen);

    // 2nd transmit
    // just get a hash...
    strcpy(buffer, "somethingelseisthishere\0");
    uint64_t hash2 = hash_djb2(buffer);
    // 2nd transmit some chunks
    strcpy(buffer, "alltest.txt\07__aabcdefghijklmnopqrstuvwx\0");
    rxhandler_add(rxm, hash2, 0, 2, buffer, buflen);
    strcpy(buffer, "babcdefghijklmnopqrstuvwx\0");
    rxhandler_add(rxm, hash2, 1, 2, buffer, buflen);
    strcpy(buffer, "cabcdefghijklmnopqrstuvwx\0");
    rxhandler_add(rxm, hash2, 2, 2, buffer, buflen);

    printf("\n>   after one incomplete and one complete insertions\n");
    rxhandler_list(rxm, print_transm, NULL);


    strcpy(buffer, "a0123456789\0");
    rxhandler_add(rxm, hash, 0, 3, buffer, buflen);
    printf("\n>   after inserting the next wrong buffer\n");
    rxhandler_list(rxm, print_transm, NULL);
    

    strcpy(buffer, "__b0123456789\0");
    rxhandler_add(rxm, hash, 1, 3, buffer, buflen);
    printf("\n>   after inserting the next right buffer\n");
    rxhandler_list(rxm, print_transm, NULL);


    transmit_t* t1 = NULL;
    rxhandler_get(rxm, hash, &t1);
    printf("\n>   the 1st hash is %lu\n", t1->hash);

    printf("\n>   getting the envelope of %p\n", t1);
    envelope_t* env;
    transmit_get_envelope(t1, &env);
    envelope_print(env);


    int error;
    envelope_writeout(env, "test/", &error);
    if (error==0)
        printf("File written: %s error:%i\n", env->name, error);
    else
        printf("File NOT written: %s error:%i\n", env->name, error);
    envelope_destroy(env);

    rxhandler_remove(rxm, hash);
    printf("\n>   after removing 1 st hash\n");
    rxhandler_list(rxm, print_transm, NULL);







    printf("\n>   done\n");
    rxhandler_destroy(rxm);

    return 0;
}



