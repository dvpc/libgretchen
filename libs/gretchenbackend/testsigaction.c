/*
catching SIGINT (ctrl-c)

simple test.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "gretchen.backend.h"




int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    grtSigcatcher_Init();

    while(!grtSigcatcher_ShouldTerminate()) {
        printf("%i ", grtSigcatcher_ShouldTerminate());
        fflush(stdout);
        sleep(1);
    }
    if (grtSigcatcher_ShouldTerminate()) {
        printf("\nCleaning up....\n");
        grtSigcatcher_Destroy();
        exit(1);
    }


    return 0;    
}



