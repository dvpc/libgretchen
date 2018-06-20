#include "gretchen.internal.h"

grtModulatorRX_t *grtModulatorRX_create(
                unsigned int shape,  
                unsigned int samples_per_symbol,
                unsigned int symbol_delay,
                float excess_bw,
                float center_rads,
                unsigned int flt_order,
                float flt_cutoff_frq,
                float flt_center_frq,
                float flt_passband_ripple,
                float flt_stopband_ripple)
{
    grtModulatorRX_t *dem = malloc(sizeof(grtModulatorRX_t));
    dem->nco = nco_crcf_create(LIQUID_NCO);
    nco_crcf_set_frequency(dem->nco,
                           center_rads);
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
            //iirfilt_rrrf_execute(
            //                dem->filter_rx, 
            //                samples[i+j],
            //                &samples[i+j]);
            agc_rrrf_execute(dem->agc, samples[i+j], &samples[i+j]);
            nco_crcf_mix_down(
                            dem->nco,
                            samples[i+j],
                            &mixer_out[j]);
            nco_crcf_step(dem->nco);
        }
        int idx = i/dem->samples_per_symbol;
        firdecim_crcf_execute(
                            dem->decim,
                            &mixer_out[0],
                            &symbols[idx]);
        symbols[idx] /= dem->samples_per_symbol;
        processed++;
    }
    return processed;
}















