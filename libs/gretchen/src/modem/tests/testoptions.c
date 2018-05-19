#include <stdlib.h>
#include <stdio.h>
#include "internal/grt.h"

int main(int argc, char** argv) {

    printf("\nif anything is fishy\ndid u call it with ... $(cat dir/optionfile)\n???\n\n");


    bool is_enc = true;
    grtModemOpt_t* opt3 = grtModemOpt_parse_args(argc, argv, is_enc); 
    printf("%p \n", opt3);
    grtModemOpt_print(opt3);
    grtModemOpt_destroy(opt3); 


    return 0;    
}



