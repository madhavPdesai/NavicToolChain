#ifndef _rf_driver_h___
#define _rf_driver_h___

void write_rf_register(uint8_t band, uint32_t reg_id, uint32_t reg_val);
uint32_t read_rf_register(uint8_t band, uint32_t reg_id);

uint32_t write_to_rf_reg(uint32_t reg_id, uint32_t wval);
uint32_t read_from_rf_reg(uint32_t reg_id);

// return 1 on error.
//    reg_id is id of 64-bit register.
int      write_u64_to_rf_reg(uint32_t reg_id, uint64_t wval);
uint64_t read_u64_from_rf_reg(uint32_t reg_id);

// Enable the engines as indicated.  Ensure that they are configured
// before being enabled.
void setRfResamplerEngineEnables (uint8_t enable_l1, uint8_t enable_l5, uint8_t enable_s);


// configure and start (as indicated) rf resampler engines in the three
// bands.
void configureRfResamplerEngine (RfResamplerEngineConfiguration* rfrec);
void configureRfResampler(RfResamplerConfiguration* rfrc);
int  verifyRfResamplerConfiguration(RfResamplerConfiguration* rfrc);
int  verifyRfResamplerEngineConfiguration(RfResamplerEngineConfiguration* rfec);

void takeRfResamplerSnapshot    (RfResamplerSnapshot* rfs);
void takeRfResamplerEngineSnapshot(uint32_t offset, RfResamplerEngineSnapshot* rfes);


// max output value of filter, given max input value..
uint32_t calculateFilterOutputMaxValue(uint32_t max_inp_value, int8_t* filter_coeffs);
// set quantization thresholds.
void setFilterQuantizationThreshold(uint8_t band, uint8_t filter_id, uint32_t quantization_threshold);
uint32_t getFilterQuantizationThreshold(uint8_t band, uint8_t filter_id);

// write RF_FILTER_ORDER filter coefficients in specified band, for specified id (1/2/3)
void setFilterStageCoefficients(uint8_t band, uint8_t filter_id, int8_t* filter_coeffs);
int  verifyFilterStageCoefficients(uint8_t band,uint8_t filter_id, int8_t *filter_1_coefficients);

// Rf engine cos-table and sine-table.
void setRfEngineCosSineTables(RfResamplerConfiguration* rfrc);
int  verifyRfEngineCosSineTables(RfResamplerConfiguration* rfrc);
int verifyRfEngineCosSineTable(int cos_flag, RfResamplerConfiguration* rfrc);


// initialize cos-table.
void initCosSineTables(int8_t* cos_table, int8_t* sine_table);

// generate parameters..
void generateRfResamplerParameters(
		double adc_if, 
		double rf_resampler_output_if, 
		// depends on the RF frontend and the upsampler.
		double incoming_sampling_frequency_after_upsampler, 
		// this is 16x1.023 MHz
		double outgoing_sampling_frequency,
		// phase advance for every input sample to resampler
		uint64_t* delta_phase_in,
		// phase advance for every output sample to resampler.
		uint64_t* delta_phase_out,
		// carrier wipeoff for 4MHz-IF and 0-IF.
		uint32_t* wipeoff_nco_phase_step,
		uint32_t* wipeoff_nco_zero_if_phase_step
		);

void setupRfResamplerEngineConfiguration(
			RfResamplerEngineConfiguration* rfec,
			int rf_band,
			double adc_sampling_frequency, 
			double adc_IF,
			int upsample_factor,
			double IF_at_resampler_output,
			double resampler_output_sampling_frequency,
			int8_t* filter_1_coeffs,
			int8_t* filter_2_coeffs,
			int8_t* filter_3_coeffs
		);
#endif
