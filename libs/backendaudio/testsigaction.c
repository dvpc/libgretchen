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

    grt_sigcatch_Init();

    while(!grt_sigcatch_ShouldTerminate()) {
        printf("%i ", grt_sigcatch_ShouldTerminate());
        fflush(stdout);
        sleep(1);
    }
    if (grt_sigcatch_ShouldTerminate()) {
        printf("\nCleaning up....\n");
        grt_sigcatch_Destroy();
        exit(1);
    }


    return 0;    
}



