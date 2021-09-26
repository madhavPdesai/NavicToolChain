//
// An application which initializes the coprocessor and rf resampler
//
//
#include <stdlib.h>
#include <stdint.h>
#include <ajit_access_routines.h>
#include <core_portme.h>
#include <math.h>
#include <navic_includes.h>
#include <app_defines.h>
// change the next line based on which filters 
// you are using.
#include <l5_filters.h>
#include <acquire.h>
#include <satellite.h>

extern NavicState volatile navic_state;
void setupRfL5Engine()
{
	// ADC sampling frequency
	double adc_sampling_frequency = L5_ADC_SAMPLING_FREQUENCY;

	// ADC sampling frequency is upsampled in the SOC by this factor.
	int    upsample_factor = L5_UPSAMPLE_FACTOR;

	// ADC IF
	double adc_if = L5_ADC_IF;

	// this is fixed to 16.368 MHz.
	double resampler_output_sampling_rate = RESAMPLER_OUTPUT_SAMPLING_FREQUENCY;

	// this is usually 4MHz but can be different!
	double IF_at_resampler_output = L5_RESAMPLER_TRACK_IF;

	PRINTF("Info: started setup L5 RF chain (band=%d).\n", RF_L5_BAND);
	setupRfResamplerEngineConfiguration(
			&(navic_state.resampler_state.configuration.l5_configuration),
			RF_L5_BAND,
			adc_sampling_frequency,
			adc_if,
			upsample_factor,
			IF_at_resampler_output,
			resampler_output_sampling_rate,
			L5_FILTER_1_COEFFS,
			L5_FILTER_2_COEFFS,
			L5_FILTER_3_COEFFS);	
	PRINTF("Info: finished setup L5 RF chain.\n");
}


