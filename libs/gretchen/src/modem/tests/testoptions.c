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



