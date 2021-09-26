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
#include <l1_filters.h>
#include <acquire.h>
#include <satellite.h>

extern NavicState volatile navic_state;
void setupRfL1Engine()
{
	// ADC sampling frequency
	double adc_sampling_frequency = L1_ADC_SAMPLING_FREQUENCY;

	// ADC sampling frequency is upsampled in the SOC by this factor.
	int    upsample_factor = L1_UPSAMPLE_FACTOR;

	// ADC IF
	double adc_if = L1_ADC_IF;

	// this is fixed to 16.368 MHz.
	double resampler_output_sampling_rate = RESAMPLER_OUTPUT_SAMPLING_FREQUENCY;

	// this is usually 4MHz but can be different!
	double IF_at_resampler_output = L1_RESAMPLER_TRACK_IF;

	PRINTF("Info: started setup L1 RF chain (band=%d).\n", RF_L1_BAND);
	setupRfResamplerEngineConfiguration(
			&(navic_state.resampler_state.configuration.l1_configuration),
			RF_L1_BAND,
			adc_sampling_frequency,
			adc_if,
			upsample_factor,
			IF_at_resampler_output,
			resampler_output_sampling_rate,
			L1_FILTER_1_COEFFS,
			L1_FILTER_2_COEFFS,
			L1_FILTER_3_COEFFS);	

	
	PRINTF("Info: finished setup L1 RF chain.\n");
}


	

