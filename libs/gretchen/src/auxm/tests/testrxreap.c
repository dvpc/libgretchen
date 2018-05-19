#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "internal/grt_aux.h"


static void print_transm(transmit_t* t, void* user)
{
    (void) user;
    transmit_print(t);
}

static void get_and_print_ripe_transmit(rxhandler_t* rxm)
{
    printf("\n>   getting ripe transmit\n");
    transmit_t* ripe;
    rxhandler_reap(rxm, &ripe);
    if (!ripe) {
        printf("ripe is NULL\n");
    } else {
        printf("ripe is %p\n", ripe);
        transmit_print(ripe);
        printf("\n");
    }
    printf(">   List of current state\n");
    rxhandler_list(rxm, print_transm, NULL);
}


int main(int argc, char **argv) {
    (void) argv;
    (void) argc;


    // TODO
    // change data type for hash to unsinged long!!!!
    // see if there are no read errors anymore in transmit_print...
    //
    // -> later change also in modem...

    // dummy data
    unsigned long hash;
    size_t buflen = 50;
    char buffer[buflen];
    memset(buffer, '\0', 50);


    printf("rx handler test\n");
    rxhandler_t* rxm = rxhandler_create();


    get_and_print_ripe_transmit(rxm);

    strcpy(buffer, "somedatawhateverkjahsdjkhsad\0");
    hash = hash_djb2((unsigned char*)buffer);


    printf("\n>   inserting chunk\n");
    strcpy(buffer, "BBBBBBBBBBBBBBB\0");
    rxhandler_add(rxm, hash, 1, 3, buffer, buflen);

    get_and_print_ripe_transmit(rxm);

    printf("\n>   inserting chunk\n");
    strcpy(buffer, "somefile.txt\07__AAAAAAAAA\0");
    rxhandler_add(rxm, hash, 0, 3, buffer, buflen);

    get_and_print_ripe_transmit(rxm);

    printf("\n>   inserting chunk\n");
    strcpy(buffer, "CCCCCCCC\0");
    rxhandler_add(rxm, hash, 2, 3, buffer, buflen);

    get_and_print_ripe_transmit(rxm);


    // FIXME
    // why????
    printf(">\n    getting it by knowing the hash works\n");
    transmit_t* t = NULL;
    rxhandler_get(rxm, hash, &t); 
    if (!t) {
        printf("t is NULL\n");
    } else {
        printf("C %p\n", t);
        transmit_print(t);
    }
    



    printf("\n>   done\n");
    rxhandler_destroy(rxm);

    return 0;
}
