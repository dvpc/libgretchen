#include <stdlib.h>
#include <stdio.h>
#include "gretchen.internal.h"

int main(int argc, char** argv) {

	grtModemOpt_t* opt3 = grtModemOpt_parse_args_from_string((uint8_t*)argv[1], true, 44100);
    //grtModemOpt_t* opt3 = grtModemOpt_parse_args_from_file((uint8_t*)argv[1], true, 44100);
    printf("%p \n", opt3);
    grtModemOpt_print(opt3);
    grtModemOpt_destroy(opt3); 


    return 0;    
}



