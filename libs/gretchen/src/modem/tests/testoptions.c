/*
 * Gretchen test modem options
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

#include <stdlib.h>
#include <stdio.h>
#include "gretchen.internal.h"

int main(int argc, char** argv) {

    /*printf("\nif anything is fishy\ndid u call it with ... $(cat dir/optionfile)\n???\n\n");*/
    grtModemOpt_t* opt3 = grtModemOpt_parse_args(argc, argv, true, 44100); 
    /*grtModemOpt_t* opt3 = grtModemOpt_parse_args_from_file((uint8_t*)argv[1], true, 44100);*/
    printf("%p \n", opt3);
    grtModemOpt_print(opt3);
    grtModemOpt_destroy(opt3); 

    printf("doing it again!!\n");
    grtModemOpt_t* opt4 = grtModemOpt_parse_args(argc, argv, true, 44100); 
    grtModemOpt_print(opt4);
    grtModemOpt_destroy(opt4);

    return 0;    
}



