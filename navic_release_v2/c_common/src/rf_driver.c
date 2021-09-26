#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <cp_rf_parameters.h>
#include <data_structures.h>
#include <rf_driver.h>
#include <macros.h>

#ifdef RF_UNIT_TEST
//
// This function must be provided by the test setup.  Allows access to
// registers when processor is not involved in the simulation.
//
void access_rf_internal_register_C_compatible (uint8_t rwbar, uint32_t reg_id, uint32_t wval, 
							uint32_t *ret_val);	
#endif 
extern int _verbose_flag;

////////////////////////////////////////////////////////////////////////////////////////////
///  Utilities                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////
uint32_t getRfRegisterIdOffset(uint8_t band)
{
	uint32_t offset = 0;
	if(band == RF_L1_BAND)
	{
		offset = RF_L1_ENGINE_ADDR_OFFSET_IN_CTRLR;
	}
	else if (band == RF_L5_BAND)
	{
		offset = RF_L5_ENGINE_ADDR_OFFSET_IN_CTRLR;
	}
	else
	{
		offset = RF_S_ENGINE_ADDR_OFFSET_IN_CTRLR;
	}
	return(offset);
}

////////////////////////////////////////////////////////////////////////////////////////////
///  Register access                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////
void write_rf_register(uint8_t band, uint32_t reg_id, uint32_t reg_val)
{
	uint32_t offset = getRfRegisterIdOffset(band);
	write_to_rf_reg(offset + reg_id, reg_val);
}

uint32_t read_rf_register(uint8_t band, uint32_t reg_id)
{
	uint32_t offset = getRfRegisterIdOffset(band);
	uint32_t ret_val = read_from_rf_reg(offset + reg_id);
	return(ret_val);
}

uint32_t write_to_rf_reg(uint32_t reg_id, uint32_t wval)
{
	uint32_t ret_val = 0;

#ifdef RF_UNIT_TEST
	 access_rf_internal_register_C_compatible (0, reg_id, wval, &ret_val);	
	if(_verbose_flag)
	{
		PRINTF("rf_reg[0x%x] = 0x%x\n", reg_id, wval);
	}
#else
	// memory mapped I/O
	uint32_t addr = P_RF_RESAMPLER_IO_MAP_BASE_ADDR + (reg_id << 2);
	*((uint32_t*) addr) = wval;
	ret_val = wval;
	if(_verbose_flag)
	{
		PRINTF("rf_reg[0x%x] = 0x%x [mem-addr=0x%x]\n", reg_id, wval, addr);
	}
#endif
	return(ret_val);
}

uint32_t read_from_rf_reg(uint32_t reg_id)
{
	uint32_t ret_val = 0;
#ifdef RF_UNIT_TEST
	access_rf_internal_register_C_compatible (1, reg_id, 0, &ret_val);	
	if(_verbose_flag)
	{
		PRINTF("0x%x = rf_reg[0x%x]\n", ret_val, reg_id);
	}
#else
	 // Memory mapped I/O
	uint32_t addr = P_RF_RESAMPLER_IO_MAP_BASE_ADDR + (reg_id << 2);
	ret_val = *((uint32_t*)addr);

	if(_verbose_flag)
	{
		PRINTF("0x%x = rf_reg[0x%x] [mem-addr=0x%x]\n", ret_val, reg_id, addr);
	}
#endif
	return(ret_val);
}

// reg_id is the id of a 64-bit register.
int      write_u64_to_rf_reg(uint32_t reg_id, uint64_t wval)
{
	write_to_rf_reg ((reg_id << 1), (wval >> 32));
	write_to_rf_reg ((reg_id << 1) | 0x1, wval & 0xffffffff);
	return(0);
}

uint64_t read_u64_from_rf_reg(uint32_t reg_id)
{
	uint32_t hw = read_from_rf_reg (reg_id << 1);
	uint32_t lw = read_from_rf_reg ((reg_id << 1) | 0x1);

	uint64_t ret_val = hw;
	ret_val = (ret_val << 32) | lw;
	return(ret_val);
}


////////////////////////////////////////////////////////////////////////////////////////////
///  Configuration                                                                        //
////////////////////////////////////////////////////////////////////////////////////////////
void configureRfResamplerEngine (RfResamplerEngineConfiguration* rfrec)
{
	if(rfrec->ignore_flag)
	{
		return;
	}

	uint32_t ctrl_word =
			(rfrec->band <<  5) | ((rfrec->wipeoff_flag & 0x1) << 4) | 
							(rfrec->input_upsample_factor & 0xf);
	uint32_t offset = getRfRegisterIdOffset (rfrec->band);
	
	// write register[0] inside the engine.
	write_to_rf_reg	( offset + RF_ENGINE_CONTROL_REGISTER_ID, ctrl_word);

	write_u64_to_rf_reg ((offset + RF_DELTA_PHASE_IN_BASE_ID) >> 1, rfrec->dphase_in);
	write_u64_to_rf_reg ((offset + RF_DELTA_PHASE_OUT_BASE_ID) >> 1, rfrec->dphase_out);

	write_to_rf_reg ((offset + RF_WIPEOFF_NCO_PHASE_STEP_BASE_ID), rfrec->wipeoff_nco_phase_step);
	write_to_rf_reg ((offset + RF_WIPEOFF_NCO_ZERO_IF_PHASE_STEP_BASE_ID), 
						rfrec->wipeoff_nco_zero_if_phase_step);

	write_to_rf_reg ((offset + RF_NUM_INPUT_SAMPLES_PER_MS_BASE_ID), rfrec->number_of_samples_per_ms_at_resampler_input);

	setFilterStageCoefficients(rfrec->band, 1, rfrec->filter_1_coefficients);
	setFilterStageCoefficients(rfrec->band, 2, rfrec->filter_2_coefficients);
	setFilterStageCoefficients(rfrec->band, 3, rfrec->filter_3_coefficients);

	uint32_t filter_1_max_output_val = calculateFilterOutputMaxValue (3, rfrec->filter_1_coefficients);

	// max input value of (sine+cos)/2
	setFilterQuantizationThreshold(rfrec->band, 2, 255);
	// low pass peak values are lower.
	setFilterQuantizationThreshold(rfrec->band, 3, 63);
}

void setFilterQuantizationThreshold(uint8_t band, uint8_t filter_id, uint32_t quantization_threshold)
{
	uint32_t offset = getRfRegisterIdOffset (band);

	if(filter_id == 2)
	{
		write_to_rf_reg (offset + RF_ENGINE_FILTER_2_OUTPUT_QUANT_THRES_REG_ID, quantization_threshold);
	}
	else if(filter_id == 3)
	{
		write_to_rf_reg (offset + RF_ENGINE_FILTER_3_OUTPUT_QUANT_THRES_REG_ID, quantization_threshold);
	}
}

uint32_t getFilterQuantizationThreshold(uint8_t band, uint8_t filter_id)
{
	uint32_t ret_val = 0;
	uint32_t offset = getRfRegisterIdOffset (band);
	if(filter_id == 2)
	{
		ret_val =	read_from_rf_reg (offset + RF_ENGINE_FILTER_2_OUTPUT_QUANT_THRES_REG_ID);
	}
	else if(filter_id == 3)
	{
		ret_val =	read_from_rf_reg (offset + RF_ENGINE_FILTER_3_OUTPUT_QUANT_THRES_REG_ID);
	}
	return(ret_val);
}

uint32_t calculateFilterOutputMaxValue(uint32_t max_inp_value, int8_t* filter_coeffs)
{
	uint32_t max_val = 0;
	int I;
	for(I =0; I <RF_FILTER_ORDER; I++)
	{
		int fc = filter_coeffs[I];
		if(fc < 0)
			fc = -fc;
		max_val += max_inp_value*fc;
	}
	return(max_val);
}



void setRfResamplerEngineEnables (uint8_t enable_l1, uint8_t enable_l5, uint8_t enable_s)
{
	uint32_t rf_ctrllr_ctrl_reg =
		(enable_l1 << 2) | (enable_l5 << 1) | enable_s;
	write_to_rf_reg (RF_CONTROLLER_CONTROL_REGISTER_ID, rf_ctrllr_ctrl_reg);
}

void configureRfResampler(RfResamplerConfiguration* rfrc)
{
	PRINTF("Info: started configuring RF resampler.\n");
	configureRfResamplerEngine (&(rfrc->l1_configuration));
	PRINTF("Info: finished configuring L1 engine.\n");
	configureRfResamplerEngine (&(rfrc->l5_configuration));
	PRINTF("Info: finished configuring L5 engine.\n");
	configureRfResamplerEngine (&(rfrc->s_configuration));
	PRINTF("Info: finished configuring S engine.\n");


	setRfEngineCosSineTables(rfrc);

	PRINTF("Info: finished configuring RF resampler.\n");
}

// write 128 filter coefficients in specified band.
void setFilterStageCoefficients(uint8_t band, uint8_t filter_id, int8_t* filter_coeffs)
{
	assert ((RF_FILTER_ORDER & 0x3) == 0);

	PRINTF("Info: started setFilterStageCoefficients (band=%d, filter_id=%d).\n",
			band, filter_id);
	uint32_t offset = 0;
	uint32_t filter_base_addr = RF_FILTER_1_COEFF_REGISTER_BASE_ID;
	if(filter_id == 2)
	{
		filter_base_addr = RF_FILTER_2_COEFF_REGISTER_BASE_ID;
	}
	else if(filter_id == 3)
	{
		filter_base_addr = RF_FILTER_3_COEFF_REGISTER_BASE_ID;
	}
	offset = getRfRegisterIdOffset (band) + filter_base_addr;

	int I;
	uint32_t wval = 0;
	for(I = 0; I <= (RF_FILTER_ORDER - 4); I += 4)
	{
		wval = filter_coeffs[I] & 0xff;
		wval = (wval << 8) | (filter_coeffs[I+1] & 0xff);
		wval = (wval << 8) | (filter_coeffs[I+2] & 0xff);
		wval = (wval << 8) | (filter_coeffs[I+3] & 0xff);

		write_to_rf_reg((offset + (I >> 2)), wval);

		if(_verbose_flag)
		{
			PRINTF("Info: setFilterStageCoefficients (band=%d, filter_id=%d), [%d] = 0x%x\n",
					band, filter_id, (offset + (I >> 2)), wval);
		}

	}
	PRINTF("Info: finished setFilterStageCoefficients (band=%d, filter_id=%d).\n",
			band, filter_id);
}

void writeCosSineTableToResampler(int cos_flag, RfResamplerConfiguration* rfrc)
{
	uint32_t table_base = (cos_flag ? RF_COS_TABLE_BASE_ID : RF_SINE_TABLE_BASE_ID);

	uint32_t offset_l1 = (RF_L1_ENGINE_ADDR_OFFSET_IN_CTRLR + table_base);
	uint32_t offset_l5 = (RF_L5_ENGINE_ADDR_OFFSET_IN_CTRLR + table_base);
	uint32_t offset_s  = (RF_S_ENGINE_ADDR_OFFSET_IN_CTRLR  + table_base);

	uint32_t table_copy_base = (cos_flag ? RF_COS_TABLE_COPY_BASE_ID : RF_SINE_TABLE_COPY_BASE_ID);

	uint32_t offset_copy_l1 = (RF_L1_ENGINE_ADDR_OFFSET_IN_CTRLR + table_copy_base);
	uint32_t offset_copy_l5 = (RF_L5_ENGINE_ADDR_OFFSET_IN_CTRLR + table_copy_base);
	uint32_t offset_copy_s  = (RF_S_ENGINE_ADDR_OFFSET_IN_CTRLR  + table_copy_base);

	int8_t* table = (cos_flag ? rfrc->cos_table : rfrc->sine_table);
	uint32_t wval = 0;

	char tt[10];
	if(cos_flag)
		sprintf(tt,"cos");
	else
		sprintf(tt,"sine");

	int I = 0;
	while(I <= ((RF_COS_TABLE_SIZE/2)- 4))
	{
		wval = table[I] & 0xff;

		I++;
		if(I < RF_COS_TABLE_SIZE/2)
			wval = (wval << 8) | (table[I] & 0xff);
		else
			wval = (wval << 8);
		I++;
		if(I < RF_COS_TABLE_SIZE/2)
			wval = (wval << 8) | (table[I] & 0xff);
		else
			wval = (wval << 8);

		I++;
		if(I < RF_COS_TABLE_SIZE/2)
			wval = (wval << 8) | (table[I] & 0xff);
		else
			wval = (wval << 8);

		if(_verbose_flag)
		{
			PRINTF("Info: setting %s_table_register[%d]=0x%x.\n", tt, I>>2, wval);
		}

		write_to_rf_reg((offset_l1 + (I >> 2)), wval);
		write_to_rf_reg((offset_l5 + (I >> 2)), wval);
		write_to_rf_reg((offset_s  + (I >> 2)), wval);

		if(_verbose_flag)
		{
			PRINTF("Info: setting %s_table_copy_register[%d]=0x%x.\n", tt, I>>2, wval);
		}
		write_to_rf_reg((offset_copy_l1 + (I >> 2)), wval);
		write_to_rf_reg((offset_copy_l5 + (I >> 2)), wval);
		write_to_rf_reg((offset_copy_s  + (I >> 2)), wval);

		I++;
	}

	PRINTF("Info: finished setting %s-table.\n", tt);
}

// Rf engine cos-table (261 entries of int8)
void setRfEngineCosSineTables(RfResamplerConfiguration* rfrc)
{
	PRINTF("Info: started setting cos-table.\n");
	writeCosSineTableToResampler(1, rfrc);
	writeCosSineTableToResampler(0, rfrc);
}


////////////////////////////////////////////////////////////////////////////////////////////
///  Verification                                                                         // 
////////////////////////////////////////////////////////////////////////////////////////////
int verifyRfResamplerConfiguration (RfResamplerConfiguration* rfrc)
{
	int err = 0;

	PRINTF("Info: started verifying resampler configuration.\n");
	int l1_err = verifyRfResamplerEngineConfiguration(&(rfrc->l1_configuration));
	PRINTF("Info: finished verifying resampler L1 engine (err = %d).\n", l1_err);
	int l5_err = verifyRfResamplerEngineConfiguration(&(rfrc->l5_configuration));
	PRINTF("Info: finished verifying resampler L5 engine (err = %d).\n", l5_err);
	int s_err  = verifyRfResamplerEngineConfiguration(&(rfrc->s_configuration));
	PRINTF("Info: finished verifying resampler S engine (err = %d).\n", s_err);

	err = err || l1_err || l5_err || s_err;
	err = verifyRfEngineCosSineTable (0, rfrc) || err;
	err = verifyRfEngineCosSineTable (1, rfrc) || err;
	return(err);
}

int verifyRfResamplerEngineConfiguration (RfResamplerEngineConfiguration* rfec)
{
	int err = 0;
	if(rfec->ignore_flag)
		return(err);

	uint32_t offset = getRfRegisterIdOffset(rfec->band);

	uint64_t dphase_in  = read_u64_from_rf_reg((offset +  RF_DELTA_PHASE_IN_BASE_ID) >> 1);
	err = (dphase_in != rfec->dphase_in) || err;

	uint64_t dphase_out = read_u64_from_rf_reg((offset +  RF_DELTA_PHASE_OUT_BASE_ID) >> 1);
	err = (dphase_out != rfec->dphase_out) || err;

	uint16_t number_of_samples_per_ms_at_resampler_input = 
			read_from_rf_reg(offset + RF_NUM_INPUT_SAMPLES_PER_MS_BASE_ID);
	err = (number_of_samples_per_ms_at_resampler_input  != 
			rfec->number_of_samples_per_ms_at_resampler_input) || err;

	uint32_t wipeoff_nco_phase_step = read_from_rf_reg(offset + RF_WIPEOFF_NCO_PHASE_STEP_BASE_ID);
	err = (wipeoff_nco_phase_step != rfec->wipeoff_nco_phase_step) || err;

	uint32_t wipeoff_nco_zero_if_phase_step = read_from_rf_reg(offset + RF_WIPEOFF_NCO_ZERO_IF_PHASE_STEP_BASE_ID);
	err = (wipeoff_nco_zero_if_phase_step != rfec->wipeoff_nco_zero_if_phase_step) || err;

	uint32_t ctrl_word = read_from_rf_reg(offset + RF_ENGINE_CONTROL_REGISTER_ID);
	uint8_t upsample_count = (ctrl_word & 0xf);
	err = (upsample_count != rfec->input_upsample_factor) || err;

	uint8_t band 	     = (ctrl_word >> 5 ) & 0x3;
	err = (band != rfec->band) || err;

	verifyFilterStageCoefficients(rfec->band,1,rfec->filter_1_coefficients);
	verifyFilterStageCoefficients(rfec->band,2,rfec->filter_2_coefficients);
	verifyFilterStageCoefficients(rfec->band,3,rfec->filter_3_coefficients);

	return(err);
}

int  verifyFilterStageCoefficients(uint8_t band,uint8_t filter_id, int8_t *filter_coeffs)
{
	assert ((filter_id >=1) && (filter_id <= 3) && ((RF_FILTER_ORDER & 0x3) == 0));
	PRINTF("Info: started verifying filter coefficients (band=%d, filter_id=%d).\n", 
				band, filter_id);

	int err = 0;

	uint32_t filter_base_addr = RF_FILTER_1_COEFF_REGISTER_BASE_ID;
	if(filter_id == 2)
	{
		filter_base_addr = RF_FILTER_2_COEFF_REGISTER_BASE_ID;
	}
	else if(filter_id == 3)
	{
		filter_base_addr = RF_FILTER_3_COEFF_REGISTER_BASE_ID;
	}
	uint32_t offset = getRfRegisterIdOffset (band) + filter_base_addr;

	int I;
	uint32_t wval = 0;
	for(I = 0; I <= (RF_FILTER_ORDER - 4); I += 4)
	{
		wval = filter_coeffs[I];
		wval = (wval << 8) | filter_coeffs[I+1];
		wval = (wval << 8) | filter_coeffs[I+2];
		wval = (wval << 8) | filter_coeffs[I+3];

		uint32_t rwval = read_from_rf_reg(offset + (I >> 2));
		err = (rwval != wval) || err;
		if(err)
			break;
	}
	PRINTF("Info: finished verifying filter coefficients (band=%d, filter_id=%d, err=%d).\n", 
				band, filter_id,err);
	return(err);
}


int verifyRfEngineCosSineTable(int cos_flag, RfResamplerConfiguration* rfrc)
{
	int err = 0;
	int I;
	uint32_t wval = 0;

	PRINTF("Info: started verifying %s table.\n", (cos_flag ? "cos" : "sin"));
	uint32_t table_base = (cos_flag ? RF_COS_TABLE_BASE_ID : RF_SINE_TABLE_BASE_ID);
	uint32_t offset_l1 = (RF_L1_ENGINE_ADDR_OFFSET_IN_CTRLR + table_base);
	uint32_t offset_l5 = (RF_L5_ENGINE_ADDR_OFFSET_IN_CTRLR + table_base);
	uint32_t offset_s  = (RF_S_ENGINE_ADDR_OFFSET_IN_CTRLR  + table_base);

	uint32_t table_copy_base = (cos_flag ? RF_COS_TABLE_COPY_BASE_ID : RF_SINE_TABLE_COPY_BASE_ID);
	uint32_t offset_copy_l1 = (RF_L1_ENGINE_ADDR_OFFSET_IN_CTRLR + table_copy_base);
	uint32_t offset_copy_l5 = (RF_L5_ENGINE_ADDR_OFFSET_IN_CTRLR + table_copy_base);
	uint32_t offset_copy_s  = (RF_S_ENGINE_ADDR_OFFSET_IN_CTRLR  + table_copy_base);

	int8_t* table = (cos_flag ? rfrc->cos_table : rfrc->sine_table);

	I = 0;
	while(I <= ((RF_COS_TABLE_SIZE/2) - 4))
	{
		wval = table[I] & 0xff;

		I++;
		if(I < (RF_COS_TABLE_SIZE/2))
			wval = (wval << 8) | (table[I] & 0xff);
		else
			wval = (wval << 8);
		I++;
		if(I < (RF_COS_TABLE_SIZE/2))
			wval = (wval << 8) | (table[I] & 0xff);
		else
			wval = (wval << 8);

		I++;
		if(I < (RF_COS_TABLE_SIZE/2))
			wval = (wval << 8) | (table[I] & 0xff);
		else
			wval = (wval << 8);

		uint32_t rl1 = read_from_rf_reg(offset_l1 + (I >> 2));
		uint32_t rl5 = read_from_rf_reg(offset_l5 + (I >> 2));
		uint32_t rs  = read_from_rf_reg(offset_s + (I >> 2));
	

		err = (rl1 != wval) || (rl5 != wval) || (rs != wval) || err;
		if(err)
		{
			PRINTF("Error: in verifying %s table (I=%d, rl1=0x%x, rl5=0x%x, rs = 0x%x, wval = 0x%x).\n", 
					(cos_flag ? "cos" : "sine"),
					I>>2, rl1, rl5, rs, wval);
			break;
		}

		rl1 = read_from_rf_reg(offset_copy_l1 + (I >> 2));
		rl5 = read_from_rf_reg(offset_copy_l5 + (I >> 2));
		rs  = read_from_rf_reg(offset_copy_s + (I >> 2));

		err = (rl1 != wval) || (rl5 != wval) || (rs != wval) || err;
		if(err)
		{
			PRINTF("Error: in verifying %s table copy (I=%d, rl1=0x%x, rl5=0x%x, rs = 0x%x, wval = 0x%x).\n", 
					(cos_flag ? "cos" : "sine"),
					I>>2, rl1, rl5, rs, wval);
			break;
		}

		I++;
	}
	PRINTF("Info: finished verifying %s table (err = %d).\n", (cos_flag ? "cos" : "sine"),  err);
	return(err);
}
						
//////////////////////////////////////////////////////////////////////////////////////////
///   Snapshots      									//
//////////////////////////////////////////////////////////////////////////////////////////
void takeRfResamplerEngineSnapshot(uint32_t offset, RfResamplerEngineSnapshot* rfes)
{
	// TODO: these need to be added to the resampler code.
	rfes->number_of_samples_received_from_adc	= 0xffffffff;
	rfes->number_of_samples_transmitted_to_memory   = 0xffffffff;
	rfes->last_8_samples_from_input_after_upsampler = 0xffff;
	rfes->last_8_samples_at_output_of_resampler = 0xffff;
}
void takeRfResamplerSnapshot (RfResamplerSnapshot* rfs)
{
	if(rfs == NULL)
		return;

	rfs->controller_ctrl_register = read_from_rf_reg (RF_CONTROLLER_CONTROL_REGISTER_ID);	


	takeRfResamplerEngineSnapshot(RF_L1_ENGINE_ADDR_OFFSET_IN_CTRLR, &(rfs->l1_engine_snapshot));
	takeRfResamplerEngineSnapshot(RF_L5_ENGINE_ADDR_OFFSET_IN_CTRLR, &(rfs->l5_engine_snapshot));
	takeRfResamplerEngineSnapshot(RF_S_ENGINE_ADDR_OFFSET_IN_CTRLR, &(rfs->s_engine_snapshot));
}

void initCosSineTables(int8_t* cos_table, int8_t* sine_table)
{
	int I; 
	for(I=0;I < RF_COS_TABLE_SIZE/2; I++)
	{
		double phase   = (2*M_PI*I/RF_COS_TABLE_SIZE);

		double cval = cos(phase);
		int8_t abs_cval_fixed_point = (int) (((1 << 7)-1)*cval);
		cos_table[I] = abs_cval_fixed_point;

		double sval = sin(phase);
		int8_t abs_sval_fixed_point = (int) (((1 << 7)-1)*sval);
		sine_table[I] = abs_sval_fixed_point;
	}

	PRINTF("Info: completed initCosSineTables.\n");
}

//  originally written by Gaurav Rathi.
void generateRfResamplerParameters(
		// IF's
		double adc_IF, 
		double rf_output_IF, 
		// depends on the RF frontend and the upsampler.
		double incoming_sampling_frequency_after_upsampler, 
		// this is 16x1.023 MHz
		double outgoing_sampling_frequency,
		// phase advance for every input sample to resampler
		uint64_t* delta_phase_in,
		// phase advance for every output sample to resampler.
		uint64_t* delta_phase_out,
		// wipeoff nco dphase
		uint32_t* wipeoff_nco_phase_step,
		// wipeoff nco dphase
		uint32_t* wipeoff_nco_zero_if_phase_step
		)
{

	// use 64 bit for resampler.
	double one = 2.0*(((uint64_t)1)<<63); 

	// this is abs(ADCIF - RFOUTIF).
	double wipeoff_frequency_internal_if = 0.0;
	double low_wo_freq  = fabs(adc_IF -  rf_output_IF);
	double high_wo_freq = adc_IF +  rf_output_IF;
	if(low_wo_freq >= 2600000.0)
		wipeoff_frequency_internal_if = low_wo_freq;
	else if(low_wo_freq != 0.0)
		wipeoff_frequency_internal_if = high_wo_freq;

	// this is ADCIF.
	double wipeoff_frequency_zero_if = adc_IF;
 
	*delta_phase_in  = (uint64_t)(one/incoming_sampling_frequency_after_upsampler);
	*delta_phase_out = (uint64_t)(one/outgoing_sampling_frequency);

	// use 32-bit for nco  good enough.
	double one_nco_rotation = (2.0*(1u << 31));
	*wipeoff_nco_phase_step = (uint32_t) 
			(wipeoff_frequency_internal_if*one_nco_rotation/incoming_sampling_frequency_after_upsampler);
	*wipeoff_nco_zero_if_phase_step = (uint32_t) 
			(wipeoff_frequency_zero_if*one_nco_rotation/incoming_sampling_frequency_after_upsampler);

	PRINTF("Info: completed generateRfResamplerParameters (wipeoff_frequencies=(%f (track),%f (acquire)), incoming-sampling-freq=%f, outgoing-sampling-freq=%f)\n",  wipeoff_frequency_internal_if,  wipeoff_frequency_zero_if,
			incoming_sampling_frequency_after_upsampler, outgoing_sampling_frequency);

}

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
		)
{
	uint64_t dphase_in;
	uint64_t dphase_out;
	uint32_t wipeoff_nco_phase_step;
	uint32_t wipeoff_nco_zero_if_phase_step;

	double in_sampling_frequency = adc_sampling_frequency * upsample_factor;

	generateRfResamplerParameters(
						adc_IF,
						IF_at_resampler_output,
						in_sampling_frequency, 
						resampler_output_sampling_frequency,
						&dphase_in, &dphase_out, &wipeoff_nco_phase_step,
						&wipeoff_nco_zero_if_phase_step);


	rfec->ignore_flag = 0; 
	rfec->band = rf_band;
	rfec->input_upsample_factor = upsample_factor;
	rfec->dphase_in = dphase_in;
	rfec->dphase_out = dphase_out;
	rfec->wipeoff_nco_phase_step = wipeoff_nco_phase_step;
	rfec->wipeoff_nco_zero_if_phase_step = wipeoff_nco_zero_if_phase_step;
	rfec->number_of_samples_per_ms_at_resampler_input = (uint32_t) in_sampling_frequency/1000;

	rfec->filter_1_coefficients = filter_1_coeffs;
	rfec->filter_2_coefficients = filter_2_coeffs;
	rfec->filter_3_coefficients = filter_3_coeffs;

	PRINTF("Info: setupResamplerEngineConfiguration: band=%d\n",rf_band);
}

