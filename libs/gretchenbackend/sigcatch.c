#include "gretchen.backend.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// signal catcher

static sigcatcher_t* sigc;

void grtSigcatcher_handler(int s) {
    (void) s;
    if (sigc)
        sigc->grtSigcatcher_should_terminate=1;
}

void grtSigcatcher_Init() {
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = grtSigcatcher_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGTERM, &sigIntHandler, NULL);
    
    if (sigc==NULL)
        sigc = malloc(sizeof(sigcatcher_t));
    if (sigc)
        sigc->grtSigcatcher_should_terminate=0;
}

int grtSigcatcher_ShouldTerminate() {
    if (sigc)
        return sigc->grtSigcatcher_should_terminate==1;
    else
        return 0;
}

void grtSigcatcher_Set(int i) {
    if (sigc)
        sigc->grtSigcatcher_should_terminate=i;
}

void grtSigcatcher_Destroy() {
    free(sigc);
    sigc=NULL;
}
