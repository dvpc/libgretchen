#include "internal/grt.h"

grtModulatorTX_t *grtModulatorTX_create(
                unsigned int shape,  
                unsigned int samples_per_symbol,
                unsigned int symbol_delay,
                float excess_bw,
                float center_rads,
                float gain,
                unsigned int flt_order,
                float flt_cutoff_frq,
                float flt_center_frq,
                float flt_passband_ripple,
                float flt_stopband_ripple,
                unsigned int flushlen_mod)
{
    grtModulatorTX_t *mod = malloc(sizeof(grtModulatorTX_t));

    mod->nco = nco_crcf_create(LIQUID_NCO);
    nco_crcf_set_frequency(mod->nco,
                           center_rads);
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
    mod->filter_tx = iirfilt_crcf_create_prototype(
                    LIQUID_IIRDES_BUTTER,
                    LIQUID_IIRDES_BANDPASS,
                    LIQUID_IIRDES_SOS,
                    flt_order,
                    flt_cutoff_frq,
                    flt_center_frq,
                    flt_passband_ripple,
                    flt_stopband_ripple);
    return mod;
}

void grtModulatorTX_destroy(
                grtModulatorTX_t *mod)
{
    if (!mod)
        return;
    nco_crcf_destroy(mod->nco);
    firinterp_crcf_destroy(mod->interp);
    iirfilt_crcf_destroy(mod->filter_tx);
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
            iirfilt_crcf_execute(
                        mod->filter_tx, 
                        v1, 
                        &v1);
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













