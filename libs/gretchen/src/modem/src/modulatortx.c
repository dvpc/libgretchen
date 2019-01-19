/*
 * Gretchen modulator tx
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

grtModulatorTX_t *grtModulatorTX_create(
                uint32_t shape,  
                uint32_t samples_per_symbol,
                uint32_t symbol_delay,
                float excess_bw,
                float freq_in_rad,
                float gain,
                uint32_t flt_order,
                float flt_cutoff_frq,
                float flt_center_frq,
                float flt_passband_ripple,
                float flt_stopband_ripple,
                uint32_t flushlen_mod)
{
    grtModulatorTX_t *mod = malloc(sizeof(grtModulatorTX_t));

    mod->nco = nco_crcf_create(LIQUID_VCO);
    nco_crcf_set_frequency(mod->nco,
                           freq_in_rad);
    nco_crcf_set_phase(mod->nco, 0.0f);
    mod->interp = firinterp_crcf_create_prototype(
                    shape,
                    samples_per_symbol,
                    symbol_delay,
                    excess_bw, 
                    0);
    mod->samples_per_symbol = samples_per_symbol;
    mod->gain = gain;
    mod->flushlen = samples_per_symbol*flushlen_mod*symbol_delay;
    /*mod->filter_tx = iirfilt_crcf_create_prototype(*/
                    /*LIQUID_IIRDES_BUTTER,*/
                    /*LIQUID_IIRDES_BANDPASS,*/
                    /*LIQUID_IIRDES_SOS,*/
                    /*flt_order,*/
                    /*flt_cutoff_frq,*/
                    /*flt_center_frq,*/
                    /*flt_passband_ripple,*/
                    /*flt_stopband_ripple);*/
    return mod;
}

void grtModulatorTX_destroy(
                grtModulatorTX_t *mod)
{
    if (!mod)
        return;
    nco_crcf_destroy(mod->nco);
    firinterp_crcf_destroy(mod->interp);
    /*iirfilt_crcf_destroy(mod->filter_tx);*/
    free(mod);
}

size_t grtModulatorTX_recv(
                grtModulatorTX_t *mod,
                float complex *symbols,
                size_t symbols_len,
                float *samples)
{
    size_t processed = 0;
    float complex mixer_out[mod->samples_per_symbol];
    for (size_t i=0; i<symbols_len; i++) {
        firinterp_crcf_execute(
                        mod->interp,
                        symbols[i],
                        &mixer_out[0]);    
        for (size_t j=0; j<mod->samples_per_symbol; j++) {
            float complex v1 = mixer_out[j];
            nco_crcf_mix_up(
                        mod->nco, 
                        v1, 
                        &v1);
            /*iirfilt_crcf_execute(*/
                        /*mod->filter_tx, */
                        /*v1, */
                        /*&v1);*/
            samples[i*mod->samples_per_symbol+j] = crealf(v1)*mod->gain; 
            nco_crcf_step(mod->nco);
            processed++;
        }
    }
    return processed;
}

size_t grtModulatorTX_flush(
                grtModulatorTX_t *mod,
                float *samples)
{
    float complex empty[mod->flushlen];
    for (size_t i=0; i<mod->flushlen; i++)
        empty[i] = 0;
    return grtModulatorTX_recv(mod,
                        empty,
                        mod->flushlen,
                        samples);
}

void grtModulatorTX_reset(
                grtModulatorTX_t *mod)
{
    firinterp_crcf_reset(mod->interp);
}













