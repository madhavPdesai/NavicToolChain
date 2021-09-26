#ifndef __iitbnavic_soc_init_h__
#define __iitbnavic_soc_init_h__

// initialize the navic soc.
void init_iitbnavic_soc(
				// which bands are enabled? (1 if enabled)
				int use_gps_l1, 
				int use_irnss_l5, 
				int use_irnss_s,
				// which adc uses the falling edge to synch its data (1 if falling)
				int l1_adc_falling_edge, 
				int l5_adc_falling_edge, 
				int s_adc_falling_edge, 
				// The l1 adc IF, Sampling-rate, upsample-factor at rf first stage.
				double l1_adc_if, 
				double l1_adc_sampling_frequency, 
				int l1_upsample_factor,
				double l1_resampler_if,
				// The l5 adc IF, Sampling-rate, upsample-factor at rf first stage.
				double l5_adc_if, 
				double l5_adc_sampling_frequency, 
				int l5_upsample_factor,
				double l5_resampler_if,
				// The s adc IF, Sampling-rate, upsample-factor at rf first stage.
				double s_adc_if, 
				double s_adc_sampling_frequency,  
				int s_upsample_factor,
				double s_resampler_if,
				// The coprocessor command buffer virtual address (usually 0x40064000).
				uint32_t cp_command_buffer_virtual_address,
				// The coprocessor command buffer physical address (usually 0x40040000).
				uint32_t cp_command_buffer_physical_address,
				// clock frequency: used to configure the serial devices.
				uint32_t clock_frequency,
				// l1 filter coefficients.
				int8_t* l1_filter_1_coeffs, 
				int8_t* l1_filter_2_coeffs, 
				int8_t* l1_filter_3_coeffs,
				// l5 filter coefficients.
				int8_t* l5_filter_1_coeffs, 
				int8_t* l5_filter_2_coeffs, 
				int8_t* l5_filter_3_coeffs,
				// s filter coefficients.
				int8_t* s_filter_1_coeffs, 
				int8_t* s_filter_2_coeffs, 
				int8_t* s_filter_3_coeffs,
				// navic state.
				NavicState* navic_state
			);
#endif
