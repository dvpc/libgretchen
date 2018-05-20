#include "gretchen.backend.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// signal catcher

static sigcatcher_t* sigc;

void grt_sigcatch_handler(int s) {
    (void) s;
    if (sigc)
        sigc->grt_sigcatch_should_terminate=1;
}

void grt_sigcatch_Init() {
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = grt_sigcatch_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGTERM, &sigIntHandler, NULL);
    
    if (sigc==NULL)
        sigc = malloc(sizeof(sigcatcher_t));
    if (sigc)
        sigc->grt_sigcatch_should_terminate=0;
}

int grt_sigcatch_ShouldTerminate() {
    if (sigc)
        return sigc->grt_sigcatch_should_terminate==1;
    else
        return 0;
}

void grt_sigcatch_Set(int i) {
    if (sigc)
        sigc->grt_sigcatch_should_terminate=i;
}

void grt_sigcatch_Destroy() {
    free(sigc);
    sigc=NULL;
}
