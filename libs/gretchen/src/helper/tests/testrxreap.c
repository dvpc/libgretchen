/*
 * Gretchen internal helper test rx handler 
 *
 * Copyright (c) 2018 - 2019 Daniel von Poschinger
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "gretchen.internal.h"


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

    // dummy data
    uint16_t hash;
    size_t buflen = 50;
    char buffer[buflen];
    memset(buffer, '\0', 50);


    printf("rx handler test\n");
    rxhandler_t* rxm = rxhandler_create();


    get_and_print_ripe_transmit(rxm);

    strcpy(buffer, "somedatawhateverkjahsdjkhsad\0");
    hash = hash_djb2((uint8_t*)buffer);


    printf("\n>   inserting chunk\n");
    strcpy(buffer, "BBBBBBBBBBBBBBB\0");
    rxhandler_add(rxm, hash, 1, 3, (uint8_t*)buffer, buflen);

    get_and_print_ripe_transmit(rxm);

    printf("\n>   inserting chunk\n");
    strcpy(buffer, "somefile.txt\07__AAAAAAAAA\0");
    rxhandler_add(rxm, hash, 0, 3, (uint8_t*)buffer, buflen);

    get_and_print_ripe_transmit(rxm);

    printf("\n>   inserting chunk\n");
    strcpy(buffer, "CCCCCCCC\0");
    rxhandler_add(rxm, hash, 2, 3, (uint8_t*)buffer, buflen);

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
