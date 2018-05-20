#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "gretchen.h"
#include "gretchen.backend.h"

static void print_usage(char* binname)
{
    printf("Gretchen version: %i.%i\nUsage: %s [[-f <file to transmit>] -o <modem options>].\n",
        gretchen_VERSION_MAJOR, gretchen_VERSION_MINOR, binname);
}

int main(int argc, char **argv) {

    bool is_modetx = false;
    bool use_defaultopt = true;
    char* txfilepath = NULL;
    char c;
    while(1) {
        c = getopt(argc, argv, "f:o:h:"); 
        if (c==-1)
            break;
        switch(c) {
            case 'f':
                is_modetx = true;
                txfilepath = optarg;
                break; 
            case 'o':
                use_defaultopt = false;
                break; 
            case 'h':
                print_usage(argv[0]);
                break; 
        default:
            print_usage(argv[0]);
        }
    }



    if (is_modetx) {
        // load file from txfilepath
        //
        // etc...
    } else {
        // start listening mode etc...
    
    
    }



    return 0;
}
