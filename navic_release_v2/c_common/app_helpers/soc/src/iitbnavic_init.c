#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "ajit_access_routines.h"
#include "core_portme.h"
#include "navic_includes.h"
#include "iitbnavic_globals.h"


void setupRfEngine(RfResamplerEngineConfiguration *config,
			uint8_t rf_band, 
			double sampling_freq, double input_if, int upsample_factor,
			double resampler_track_if,
			int8_t* filter_1_coeffs, int8_t* filter_2_coeffs, int8_t* filter_3_coeffs)
{
	// ADC sampling frequency
	double adc_sampling_frequency = adc_sampling_frequency;

	// ADC IF
	double adc_if = input_if;

	// this is fixed to 16.368 MHz.
	double resampler_output_sampling_rate = IITBNAVIC_RESAMPLER_OUTPUT_SAMPLING_FREQUENCY;

	// this is usually 4MHz but can be different!
	double IF_at_resampler_output = resampler_track_if;

	setupRfResamplerEngineConfiguration(
			config,
			rf_band,
			adc_sampling_frequency,
			adc_if,
			upsample_factor,
			IF_at_resampler_output,
			resampler_output_sampling_rate,
			filter_1_coeffs,
			filter_2_coeffs,
			filter_3_coeffs);	
	PRINTF("Info: finished setup RF chain (band=%d).\n", rf_band);
}


//
// Initializes the SOC and enables interrupts on the coprocessor.
//
// After this function is called, the coprocessor will start generating
// an interrupt every 1 ms.
//
void init_iitbnavic_soc (
				int use_gps_l1, int use_irnss_l5, int use_irnss_s,
				int l1_adc_falling_edge, int l5_adc_falling_edge, int s_adc_falling_edge,
				double l1_adc_if, 
				double l1_adc_sampling_frequency, 
				int l1_upsample_factor,
				double l1_resampler_track_if,
				double l5_adc_if, 
				double l5_adc_sampling_frequency, 
				int l5_upsample_factor,
				double l5_resampler_track_if,
				double s_adc_if, 
				double s_adc_sampling_frequency,  
				int s_upsample_factor,
				double s_resampler_track_if,
				uint32_t cp_command_buffer_virtual_address,
				uint32_t cp_command_buffer_physical_address,
				uint32_t clock_frequency,
				int8_t* l1_filter_1_coeffs, int8_t* l1_filter_2_coeffs, int8_t* l1_filter_3_coeffs,
				int8_t* l5_filter_1_coeffs, int8_t* l5_filter_2_coeffs, int8_t* l5_filter_3_coeffs,
				int8_t* s_filter_1_coeffs, int8_t* s_filter_2_coeffs, int8_t* s_filter_3_coeffs,
				NavicState* navic_state
			)
{
	// set baudrates for uarts connected to the core.
	__ajit_serial_set_baudrate_via_vmap__ (115200, clock_frequency);

	// enable AJIT processor serial device.
	__ajit_write_serial_control_register__ (TX_ENABLE);

	// enable AJIT processor interrupt controller.
	__ajit_write_irc_control_register__(1);


	CoprocessorState* cp_state = (CoprocessorState*) &(navic_state->coprocessor_state);

	// turn off useless messages.
	setCpLinkVerboseFlag(0);

	PRINTF("Info: started initializing coprocessor.\n");
	initializeCoprocessor(cp_state,
			// coprocessor-index
			0,
			// command buffer virtual address.
			cp_command_buffer_virtual_address,  
			// command buffer physical address.
			cp_command_buffer_physical_address,
			// 64 satellites 32xGPS, 14xuse_irnss_l5, 14xuse_irnss_s, 3 dummies.
			IITBNAVIC_NUMBER_OF_SUPPORTED_SATELLITES,
			// 16.368 MHz.
			IITBNAVIC_RESAMPLER_OUTPUT_SAMPLING_FREQUENCY,
			//
			// these two fields are not used
			// in normal operation.
			//
			clock_frequency/1000,
			// do not generate command strobe automatically
			// it will be generated in the interrupt handler.
			(clock_frequency/1000) + 1);

	PRINTF("Info: virtual address of command_response_array is 0x%x\n",
			(uint32_t) navic_state->coprocessor_state.command_response_array);
	PRINTF("Info: virtual address of command_response_array[0].args[0]) is 0x%x\n",
			(uint32_t) &(navic_state->coprocessor_state.command_response_array[0].args[0]));

	///////////////////////////////////////////////////////////////////////////////////////////
	// Initialize RF chain
	///////////////////////////////////////////////////////////////////////////////////////////
	PRINTF("Info: started initializing RF chains.\n");
	setCoprocessorAdcClocking(l1_adc_falling_edge, l5_adc_falling_edge, s_adc_falling_edge);

	if(use_gps_l1)
	{
		setupRfEngine(&(navic_state->resampler_state.configuration.l1_configuration), 
					RF_L1_BAND,
					l1_adc_sampling_frequency, l1_adc_if, l1_upsample_factor,
					l1_resampler_track_if,
					l1_filter_1_coeffs, l1_filter_2_coeffs, l1_filter_3_coeffs);
	}
	else
	{
		navic_state->resampler_state.configuration.l1_configuration.ignore_flag = 1;
	}

	if(use_irnss_l5)
	{
		setupRfEngine(&(navic_state->resampler_state.configuration.l5_configuration), 
					RF_L5_BAND,
					l5_adc_sampling_frequency, l5_adc_if, l5_upsample_factor,
					l5_resampler_track_if,
					l5_filter_1_coeffs, 
					l5_filter_2_coeffs, 
					l5_filter_3_coeffs);
	}
	else
	{
		navic_state->resampler_state.configuration.l5_configuration.ignore_flag = 1;
	}

	if(use_irnss_s)
	{
		setupRfEngine(&(navic_state->resampler_state.configuration.s_configuration), 
					RF_S_BAND,
					s_adc_sampling_frequency, s_adc_if, s_upsample_factor,
					s_resampler_track_if,
					s_filter_1_coeffs, 
					s_filter_2_coeffs, 
					s_filter_3_coeffs);
	}
	else
	{
		navic_state->resampler_state.configuration.s_configuration.ignore_flag = 1;
	}


	// initialize the sine/cosine tables.
	initCosSineTables(navic_state->resampler_state.configuration.cos_table,
				navic_state->resampler_state.configuration.sine_table);

	configureRfResampler(&(navic_state->resampler_state.configuration));
	PRINTF("Info: finished configuring RF resampler.\n");

	verifyRfResamplerConfiguration(&(navic_state->resampler_state.configuration));
	PRINTF("Info: finished verifying RF resampler.\n");

	setRfResamplerEngineEnables(use_gps_l1,use_irnss_l5,use_irnss_s);
	PRINTF("Info: enabled RF engine (%d,%d,%d).\n", use_gps_l1, use_irnss_l5, use_irnss_s);

	///////////////////////////////////////////////////////////////////////////////////////////
	// Set the limit on the size of ms data... 0 means free-running.
	///////////////////////////////////////////////////////////////////////////////////////////
	setRfMaxMsCount (0,0,0);

	// enable the RF memory interfaces.
	uint32_t toggle_mask = 
		(use_gps_l1 	? CP_TOGGLE_RF_L1_ENABLE_MASK : 0) |
		(use_irnss_l5 	? CP_TOGGLE_RF_L5_ENABLE_MASK : 0) |
		(use_irnss_s 	? CP_TOGGLE_RF_S_ENABLE_MASK : 0);
	writeCoprocessorControlWord (toggle_mask);
	PRINTF("Info: toggled RF mem interface enables.\n");

	// enable interrupts.
	writeCoprocessorControlWord (CP_ENABLE_MASK | CP_INTR_ENABLE_MASK);
	PRINTF("Info: enabled CP interrupts.\n");
}


