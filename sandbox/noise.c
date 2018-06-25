// TODO
// better use this http://liquidsdr.org/doc/channel/

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <liquid/liquid.h>
#include <time.h>

int main(int argc, char **argv) {

    FILE *input = stdin;
    FILE *output = stdout;

    float noise_floor = -40.0f;  // noise floor
    float SNRdB       = 20.0f;   // signal-to-noise ratio [dB]

    char c;
    while (1) {
        c = getopt (argc, argv, "f:r:g");
        if (c == -1)
            break;
        switch (c) {
            case 'f':
                noise_floor = atof(optarg);
                break;
            case 'r':
                SNRdB = atof(optarg);
                break;
            case '?':
                break;
        default:
            printf ("Usage: %s [-f <noise floor dB>] [-r <gain>].\n", argv[0]);
        }
    }
    
    srand(time(NULL));
    float nstd = powf(10.0f, noise_floor/20.0f);              // noise std. dev.
    /*float gamma = powf(10.0f, (SNRdB+noise_floor)/20.0f);   // channel gain*/
    float gamma = SNRdB;
    size_t readbuf_len = 1 << 10;
    float* readbuf = malloc(readbuf_len * sizeof(float));


    while(true) {
        size_t nread = fread(readbuf, sizeof(float), readbuf_len, input);
    
        // do the noise here 
        for (size_t i=0; i<nread; i++) {
            readbuf[i] += nstd*randnf()*M_SQRT1_2; 
            readbuf[i] *= gamma;
        } 
        
        fwrite(readbuf, sizeof(float), nread, output);
        if (nread == 0)
            break;
    }
    free(readbuf);
    return 0;
}
