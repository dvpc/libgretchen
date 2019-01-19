/*
 * Gretchen modulator rx
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

#include "gretchen.internal.h"

grtModulatorRX_t *grtModulatorRX_create(
                uint32_t shape,  
                uint32_t samples_per_symbol,
                uint32_t symbol_delay,
                float excess_bw,
                float freq_in_rad,
                uint32_t flt_order,
                float flt_cutoff_frq,
                float flt_center_frq,
                float flt_passband_ripple,
                float flt_stopband_ripple)
{
    grtModulatorRX_t *dem = malloc(sizeof(grtModulatorRX_t));
    dem->nco = nco_crcf_create(LIQUID_VCO);
    nco_crcf_set_frequency(dem->nco,
                           freq_in_rad);
    nco_crcf_set_phase(dem->nco, 0.0f);
    dem->decim = firdecim_crcf_create_prototype(
                    shape,
                    samples_per_symbol,
                    symbol_delay,
                    excess_bw,
                    0);
    dem->samples_per_symbol = samples_per_symbol;
    /*dem->filter_rx = iirfilt_rrrf_create_prototype(*/
                    /*LIQUID_IIRDES_BUTTER,*/
                    /*LIQUID_IIRDES_BANDPASS,*/
                    /*LIQUID_IIRDES_SOS,*/
                    /*flt_order,*/
                    /*flt_cutoff_frq,*/
                    /*flt_center_frq,*/
                    /*flt_passband_ripple,*/
                    /*flt_stopband_ripple);*/
    dem->agc = agc_rrrf_create();
    agc_rrrf_set_bandwidth(dem->agc, 0.25f);
    return dem;
}

void grtModulatorRX_destroy(
                grtModulatorRX_t *dem)
{
    if (!dem)
        return;
    nco_crcf_destroy(dem->nco);
    firdecim_crcf_destroy(dem->decim);
    /*iirfilt_rrrf_destroy(dem->filter_rx);*/
    agc_rrrf_destroy(dem->agc);
    free(dem);
}

size_t grtModulatorRX_recv(
                grtModulatorRX_t *dem,
                float *samples, 
                size_t samples_len, 
                float complex *symbols)
{
    size_t processed = 0;
    float complex mixer_out[dem->samples_per_symbol];
    for(size_t i=0; i<samples_len; i+=dem->samples_per_symbol) {
        for(size_t j=0; j<dem->samples_per_symbol; j++) {
            agc_rrrf_execute(
                            dem->agc,
                            samples[i+j], 
                            &samples[i+j]);
            /*iirfilt_rrrf_execute(*/
                            /*dem->filter_rx, */
                            /*samples[i+j],*/
                            /*&samples[i+j]);*/
            nco_crcf_mix_down(
                            dem->nco,
                            samples[i+j],
                            &mixer_out[j]);
            nco_crcf_step(dem->nco);
        } 
        uint32_t idx = (int)(i/dem->samples_per_symbol);
        firdecim_crcf_execute(
                            dem->decim,
                            &mixer_out[0],
                            &symbols[idx]);
        symbols[idx] /= dem->samples_per_symbol;
        processed++;
    }
    return processed;
}















