/*
 * Gretchen test modulator
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
#include <complex.h>
#include <time.h>
#include <math.h>

#include "gretchen.internal.h"

static size_t generate_test_cf_symbols(int num_elements,
                                     float max_val,
                                     float complex *outvalues,
                                     size_t outvalues_len)
{
    srand(time(NULL));
    /*printf("num: %i max: %f\n", num_elements, max_val);*/
    // if num_values is odd we reduce it by one
    if (num_elements % 2 != 0)
        num_elements --;
    float delta_val = max_val / ((float)num_elements*.5);
    /*printf("delta: %f\n", delta_val);*/
    // generate values for later use
    int maxidx = num_elements/2;
    float values[num_elements];
    int idx = 0;
    for(int32_t i=-maxidx; i<=maxidx; i++) {
        values[idx] = delta_val*i;
        /*printf("%12.4e \n", values[idx]);*/
        idx ++;
    }
    // set num_elements to the actual number
    num_elements = idx;
    // now write as much values to outvalues as it can
    // leaving the rest untouched.
    // generate complex of (a,0) or (0,a) or (a,a) where a 
    // is a value generated above
    size_t written = 0;
    for (size_t j=0; j<outvalues_len; j++) {
        if (j>(size_t)num_elements)
            break;
        float complex c; 
        /*c = (randnf() + randnf()*_Complex_I) + 3.0f*cexpf(-_Complex_I*0.2f*j);*/
        int r = rand() % 2;
        if (r==0)
            c = randnf()*values[j] + _Complex_I*0.0f;
        else
            c = 0.0f + _Complex_I*randnf()*values[j];
        outvalues[j] = c;
        written ++;
    }
    return written;
}




int main(int argc, char** argv) {
    (void) argc, (void) argv;

    int order = 0;


    size_t num = 900;
    float complex *testvals = calloc(num, sizeof(float complex));
    generate_test_cf_symbols(950, 5, testvals, num);

    /*uint32_t shape = liquid_getopt_str2firfilt("rrcos");  */
    uint32_t shape = liquid_getopt_str2firfilt("arkaiser");
    uint32_t samples_per_symbol = 5;
    uint32_t symbol_delay = 17;
    float excess_bw = 0.15;
    float freq_in_rad = (18500.0f/44110.0f) * 2*M_PI;
    /*float freq_in_rad = 0.4f * 2*M_PI;//18200;*/
    float gain = 0.15;
    uint32_t flushlen_mod = 5;
    grtModulatorTX_t *mod = grtModulatorTX_create(shape, samples_per_symbol, symbol_delay, excess_bw, freq_in_rad, gain, 6, .25f, .45f, 1.0f, 60.0f, flushlen_mod);
    grtModulatorRX_t *dem = grtModulatorRX_create(shape, samples_per_symbol, symbol_delay, excess_bw, freq_in_rad, 7, .3f, .36f, 1.0f, 60.0f);



    size_t samples_len = mod->samples_per_symbol*num;
    float *samples = calloc(samples_len, sizeof(float));
    size_t smpl_wr = grtModulatorTX_recv(mod, testvals, num, samples);

    printf("samples written: %12.4zu %12.4zu \n", smpl_wr, samples_len);

    // FIXME 
    size_t symbols_len = num;
    float complex *symbols = calloc(symbols_len, sizeof(float complex));
    size_t sym_wr = grtModulatorRX_recv(dem, samples, smpl_wr, symbols); 

    printf("symbols written: %12.4zu %12.4zu \n", sym_wr, symbols_len);


    FILE * fid = fopen("test.m","w");
    fprintf(fid,"%% %s : auto-generated file\n", "test.m");
    fprintf(fid,"clear all;\n");
    fprintf(fid,"close all;\n");
    fprintf(fid,"\n");
    fprintf(fid,"order=%u;\n", order);
    fprintf(fid,"n=%zu;\n",samples_len);
    fprintf(fid,"x=zeros(1,n);\n");
    fprintf(fid,"y=zeros(1,n);\n");
    fprintf(fid,"z=zeros(1,n);\n");

    for (size_t i=0; i<samples_len; i++) {
        if (i<num) {
            fprintf(fid,"x(%4lu) = %12.4e + j*%12.4e;\n", i+1, crealf(testvals[i]), cimagf(testvals[i]));
            fprintf(fid,"y(%4lu) = %12.4e + j*%12.4e;\n", i+1, crealf(symbols[i]), cimagf(symbols[i]));
        }
        else {
            fprintf(fid,"x(%4lu) = %12.4e + j*%12.4e;\n", i+1, 0.0f, 0.0f);
            fprintf(fid,"y(%4lu) = %12.4e + j*%12.4e;\n", i+1, 0.0f, 0.0f);
        }
        fprintf(fid,"z(%4lu) = %12.4e;\n", i+1, samples[i]);
    }
    fprintf(fid,"t=0:(n-1);\n");
    fprintf(fid,"figure;\n");
    fprintf(fid,"subplot(3,1,1);\n");
    fprintf(fid,"  plot(t,real(x),'-','Color',[1 0 1]*0.5,'LineWidth',1,...\n");
    fprintf(fid,"       t,imag(x),'-','Color',[0 1 1]*0.5,'LineWidth',1);\n");
    fprintf(fid,"  xlabel('time');\n");
    fprintf(fid,"  ylabel('real/imag');\n");
    fprintf(fid,"  legend('input real','imag','location','northeast');\n");
    fprintf(fid,"  grid on;\n");
    fprintf(fid,"subplot(3,1,2);\n");
    fprintf(fid,"  plot(t,real(y),'-','Color',[1 0 1]*0.5,'LineWidth',1,...\n");
    fprintf(fid,"       t,imag(y),'-','Color',[0 1 1]*0.5,'LineWidth',1);\n");
    fprintf(fid,"  xlabel('time');\n");
    fprintf(fid,"  ylabel('real/imag');\n");
    fprintf(fid,"  legend('output real','imag','location','northeast');\n");
    fprintf(fid,"  grid on;\n");
    fprintf(fid,"subplot(3,1,3);\n");
    fprintf(fid,"  plot(t,z,'-','Color',[1 1 1]*0.5,'LineWidth',1);\n");
    fprintf(fid,"  xlabel('time');\n");
    fprintf(fid,"  ylabel('sample');\n");
    fprintf(fid,"  legend('sample','location','northeast');\n");
    fprintf(fid,"  grid on;\n");

    fclose(fid);



    free(samples);
    free(testvals);
    grtModulatorTX_destroy(mod);
    grtModulatorRX_destroy(dem);
    return 0;    
}

