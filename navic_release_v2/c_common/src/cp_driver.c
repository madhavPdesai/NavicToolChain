#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <macros.h>
#include <cp_rf_parameters.h>
#include <data_structures.h>
#include <cp_internals.h>
#include <cp_driver.h>
#include <cp_link.h>
#include <all_sats_prn_gen.h>

extern CoprocessorState coprocessor_state;

void toggleActiveRfBlockId()
{
	write_to_cp_reg (CP_CONTROL_REGISTER_ID, (1 << 5));
}

int toggleTest()
{
	int err = 0;
	// toggle bit 16 
	write_to_cp_reg (CP_CONTROL_REGISTER_ID, (1 << 5));
	uint32_t cp_reg_val = read_from_cp_reg(CP_CONTROL_REGISTER_ID);
	if(cp_reg_val != (1 << 16))
	{
		PRINTF("Error: toggle[16] expected cpreg=0x%x, found 0x%x\n",
				(1 << 16), 
				cp_reg_val);
		err = 1;
	}
	write_to_cp_reg (CP_CONTROL_REGISTER_ID, (1 << 5));
	cp_reg_val = read_from_cp_reg(CP_CONTROL_REGISTER_ID);
	if(cp_reg_val != 0)
	{
		PRINTF("Error: toggle[16] expected cpreg=0, found 0x%x\n",
				cp_reg_val);
		err = 1;
	}
	return(err);
}

void writeCoprocessorControlWord(uint32_t control_mask)
{
	write_to_cp_reg(CP_CONTROL_REGISTER_ID, control_mask);
}

uint32_t readCoprocessorControlWord()
{
	uint32_t ret_val = read_from_cp_reg(CP_CONTROL_REGISTER_ID);
	return(ret_val);
}

// clear interrupt.
void clearInterrupt()
{
	uint32_t ret_val = read_from_cp_reg (CP_CONTROL_REGISTER_ID);
	write_to_cp_reg (CP_CONTROL_REGISTER_ID, ((ret_val & 0x3) | CP_CLEAR_INTR_MASK));
}

// return 0x0 if not legal.
uint32_t get_rf_reg_id (uint8_t acq_flag, uint8_t band, uint8_t block_id, uint32_t offset)
{
	uint32_t reg_id = 0x0;
	uint32_t block_size = (acq_flag ? RF_ACQ_BLOCK_SIZE : RF_TRACK_BLOCK_SIZE);
	uint32_t base = 0;
	offset = offset + ((block_id == 0) ? 0 : block_size);
	if(band == RF_L1_BAND)
	{
		base = (acq_flag ? RF_ACQ_L1_REGISTERS_ADDR_MIN : RF_L1_REGISTERS_ADDR_MIN);
	}
	else if(band == RF_L5_BAND)
	{
		base = (acq_flag ? RF_ACQ_L5_REGISTERS_ADDR_MIN : RF_L5_REGISTERS_ADDR_MIN);
	}
	else if(band == RF_S_BAND)
	{
		base = (acq_flag ? RF_ACQ_S_REGISTERS_ADDR_MIN : RF_S_REGISTERS_ADDR_MIN);
	}

	reg_id = (base + offset);

	// check overflow.
	if (reg_id >= (base + (2*block_size)))
		reg_id = 0x0;

	return(reg_id);
}

void writeCoprocessorRfWord(uint8_t acq_flag, 
		uint8_t band, uint8_t block_id, uint32_t offset, uint32_t rf_word)
{
	uint32_t reg_id = get_rf_reg_id (acq_flag, band, block_id, offset);
	if(reg_id != 0)
	{
		write_to_cp_reg(reg_id, rf_word);
	}
}


uint32_t readCoprocessorRfWord(uint8_t acq_flag,
		uint8_t band, uint8_t block_id, uint32_t offset)
{
	uint32_t rf_word = 0;
	uint32_t reg_id = get_rf_reg_id (acq_flag, band, block_id, offset);
	if(reg_id != 0)
	{
		rf_word = read_from_cp_reg(reg_id);
	}
	return(rf_word);
}

void clearAcquireRfBuffers(uint8_t rf_band)
{
	int I;
	uint32_t block_size = RF_ACQ_BLOCK_SIZE;
	for(I=0; I < block_size; I++)
	{
		writeCoprocessorRfWord(1,rf_band, 0, I, 0x0);
		writeCoprocessorRfWord(1,rf_band, 1, I, 0x0);
	}
}
void clearTrackRfBuffers(uint8_t rf_band)
{
	int I;
	uint32_t block_size = RF_TRACK_BLOCK_SIZE;
	for(I=0; I < block_size; I++)
	{
		writeCoprocessorRfWord(0,rf_band, 0, I, 0x0);
		writeCoprocessorRfWord(0,rf_band, 1, I, 0x0);
	}
}

void setRfMaxMsCount(uint8_t l1_max, uint8_t l5_max, uint8_t s_max)
{
	uint32_t word = (((uint32_t) s_max) << 16) | (((uint32_t) l5_max) << 8) | l1_max;
	write_to_cp_reg (CP_RF_MS_LIMIT_REGISTER_ID, word);
}

void getRfMsCounts (uint32_t* l1_count, uint32_t* l5_count, uint32_t* s_count)
{
	*l1_count = read_from_cp_reg (CP_RF_L1_MS_COUNTER_REGISTER_ID);
	*l5_count = read_from_cp_reg (CP_RF_L5_MS_COUNTER_REGISTER_ID);
	*s_count  = read_from_cp_reg (CP_RF_S_MS_COUNTER_REGISTER_ID);
}

void setRfMsCount(uint8_t rf_band, uint32_t count)
{
	if(rf_band == RF_L1_BAND)
		write_to_cp_reg (CP_RF_L1_MS_COUNTER_REGISTER_ID, count);
	else if (rf_band == RF_L5_BAND)
		write_to_cp_reg (CP_RF_L5_MS_COUNTER_REGISTER_ID, count);
	else if (rf_band == RF_S_BAND)
		write_to_cp_reg (CP_RF_S_MS_COUNTER_REGISTER_ID, count);
}

void probeAcquireCommandStatus(uint32_t* cell_count, uint8_t* command_id,  uint8_t* started, uint8_t *finished, uint8_t* waiting_for_rf)
{
	uint32_t status_reg = read_from_cp_reg (CP_ACQUIRE_COMMAND_STATUS_REG_ID);
	*cell_count = (status_reg >> 9) & 0x3ffff; // 22 bits.
	*command_id = (status_reg >> 3) & 0x3f;    // 6 bits
	*started    = (status_reg >> 2) & 0x1;
	*finished   = (status_reg >> 1) & 0x1;
	*waiting_for_rf    = status_reg & 0x1;
}

void probeTrackCommandStatus(uint8_t* command_id,  uint8_t* started, uint8_t *finished)
{
	uint32_t status_reg = read_from_cp_reg (CP_TRACK_COMMAND_STATUS_REG_ID);
	*command_id = (status_reg >> 2) & 0x3f;    // 6 bits
	*started    = (status_reg >> 1) & 0x1;
	*finished   = status_reg & 0x1;
}

int  initializeCoprocessor(CoprocessorState* cp_state,
		uint32_t coprocessor_index,

		// cp_state->command_response_array = pointer.
		uint32_t virtual_pointer_to_command_array,
		uint32_t physical_pointer_to_command_array,

		// max number of commands (at most 64)
		uint32_t max_number_of_commands,

		// internal sampling frequency
		double internal_sampling_frequency,

		// in units of clock cycles.
		uint32_t interrupt_interval, 

		// in units of clock cycles.
		uint32_t phase_1_duration)
{
	int err = 0;
	write_to_cp_reg(CP_CONTROL_REGISTER_ID, 0x0);
	uint32_t cp_reg_val = read_from_cp_reg(CP_CONTROL_REGISTER_ID);
	if(cp_reg_val != 0)
	{
		PRINTF("Error: expected cpreg=0, found 0x%x\n",cp_reg_val);
	}
	else
	{
		err = toggleTest() || err;
	}

	if(err)
		return(1);
	else
		PRINTF("Info: toggle test passed\n");

	// set up the command region pointer and initialize it.
	cp_state->command_response_array =  
			(CpCommandResponseRecord*) virtual_pointer_to_command_array;
	cp_state->physical_address_of_command_response_array =   physical_pointer_to_command_array;

	int I;
	// command array is 8x64 = 512 dwords. 
	uint64_t* ptr = (uint64_t*) virtual_pointer_to_command_array;
	PRINTF("initializeCoprocessor: command vptr = 0x%x, pptr = 0x%x\n", 
					(uint32_t) cp_state->command_response_array,
					(uint32_t) cp_state->physical_address_of_command_response_array);
	for(I = 0; I < 512; I++)
	{
		ptr[I] = (~I);
	}

	for(I = 0; I < 512; I++)
	{
		if(ptr[I] != (~I))
		{
			PRINTF("Error: in command buffer march test\n");
		}
		ptr[I] = 0;
	}


	cp_state->coprocessor_index = coprocessor_index;
	cp_state->max_number_of_commands = max_number_of_commands;

	cp_state->internal_sampling_frequency = internal_sampling_frequency;
	cp_state->track_samples_per_ms = (int32_t) (internal_sampling_frequency * 0.001);
	if(cp_state->track_samples_per_ms & 0x1)
		cp_state->track_samples_per_ms += 1;
	PRINTF("Info: Coprocessor track samples per ms = %d\n", cp_state->track_samples_per_ms);


	cp_state->internal_acquire_sampling_frequency = internal_sampling_frequency/4;
	cp_state->acquire_samples_per_ms = (int32_t) (cp_state->internal_acquire_sampling_frequency * 0.001);
	if(cp_state->acquire_samples_per_ms & 0x1)
		cp_state->acquire_samples_per_ms += 1;
	PRINTF("Info: Coprocessor acquire samples per ms = %d\n", cp_state->acquire_samples_per_ms);

	cp_state->interrupt_counter = 0;
	cp_state->interrupt_interval = interrupt_interval;
	cp_state->phase_1_duration = phase_1_duration;
	
	
	write_to_cp_reg(CP_SHARED_MEMORY_COMMAND_REGN_BASE_REGISTER_ID,  
					(uint32_t) physical_pointer_to_command_array);
	write_to_cp_reg(CP_SHARED_MEMORY_MAX_COMMAND_COUNT_REGISTER_ID, 
					cp_state->max_number_of_commands);
	write_to_cp_reg(CP_INTERRUPT_INTERVAL_REGISTER_ID,
					cp_state->interrupt_interval);
	write_to_cp_reg(CP_INTERRUPT_PHASE_1_INTERVAL_REGISTER_ID,
					cp_state->phase_1_duration);

#define CHECKBACK__(rid,val) {\
	uint32_t v = read_from_cp_reg(rid);\
	if (v != val)\
	{\
		PRINTF("Error: reg[%d] != 0x%x\n",rid,val);\
		err = 1;\
	}\
	}


	CHECKBACK__(CP_SHARED_MEMORY_COMMAND_REGN_BASE_REGISTER_ID,  
					((uint32_t) physical_pointer_to_command_array));
	CHECKBACK__(CP_SHARED_MEMORY_MAX_COMMAND_COUNT_REGISTER_ID, 
					cp_state->max_number_of_commands);
	CHECKBACK__(CP_INTERRUPT_INTERVAL_REGISTER_ID,
					cp_state->interrupt_interval);
	CHECKBACK__(CP_INTERRUPT_PHASE_1_INTERVAL_REGISTER_ID,
					cp_state->phase_1_duration);

	return(err);
}

void enableCoprocessor(CoprocessorState* cp_state, uint8_t interrupt_flag)
{
	uint32_t wval = 1 | (interrupt_flag << 1);
	write_to_cp_reg(CP_CONTROL_REGISTER_ID, wval);
}

void setCoprocessorAdcClocking (uint8_t l1_falling_edge, uint8_t l5_falling_edge, uint8_t s_falling_edge)
{
	PRINTF("Warning: setCoprocessorAdcClocking ignored... sampling will be on rising edges only.\n");
	write_to_cp_reg (CP_RF_ADC_CLOCKING_REG_ID, 0x0);					
}

void getCoprocessorAdcClocking (uint8_t* l1_falling_edge, uint8_t* l5_falling_edge, uint8_t* s_falling_edge)
{
	uint32_t val = read_from_cp_reg (CP_RF_ADC_CLOCKING_REG_ID);

	*l1_falling_edge = val & 0x1;
	*l5_falling_edge = (val >> 1) & 0x1;
	*s_falling_edge = (val >> 2) & 0x1;
}


// if you wish to independently set or get the intervals.
void setCoprocessorInterruptIntervals(CoprocessorState* cp_state,
						uint32_t interrupt_interval, uint32_t phase_1_length)
{
	write_to_cp_reg(CP_INTERRUPT_INTERVAL_REGISTER_ID,
					cp_state->interrupt_interval);
	write_to_cp_reg(CP_INTERRUPT_PHASE_1_INTERVAL_REGISTER_ID,
					cp_state->phase_1_duration);
}
// values returned by reference.
void getCoprocessorInterruptIntervals(CoprocessorState* cp_state,
						uint32_t *interrupt_interval, uint32_t* phase_1_duration)
{
	*interrupt_interval = read_from_cp_reg(CP_INTERRUPT_INTERVAL_REGISTER_ID);
	*phase_1_duration   = read_from_cp_reg(CP_INTERRUPT_PHASE_1_INTERVAL_REGISTER_ID);
}

CpCommandDaemonStatus getCoprocessorCommandDaemonStatus(CoprocessorState* cp_state)
{
	CpCommandDaemonStatus ret_stat = UNDEFINED;
	uint32_t CR = read_from_cp_reg(0);
	CR = (CR >> 10) & 0x3;
	if(CR == 2)
		ret_stat = STARTED_SWEEP;
	else if(CR == 1)
		ret_stat = COMPLETED_SWEEP;

	return(ret_stat);
}


int activateSatellite (CoprocessorState* cp_state,
					uint8_t satellite_id, 
					uint8_t band, 
					int8_t* prn_sequence)
{
	int I;
	int J;

	uint64_t dword = 0;
	uint32_t base_address_u32 = 
			(PRN_REGISTERS_ADDR_MIN + (satellite_id * 32));
	uint32_t base_addr_u64 = (base_address_u32 >> 1);
	uint32_t sat_offset = (satellite_id * 1023);

	for(I=0; I < 1023; I++)
	{
		if((I % 64) == 0)
		{
			if(I > 0) 
			{
				J = ((I/64) - 1);
			 	write_u64_to_cp_reg (base_addr_u64 + J, dword);
			}
			dword = 0;
		}

		dword = (dword << 1)  | ((prn_sequence[sat_offset + I] == 1) ? 0x0 : 0x1);
	}

	J = 15;
	dword = (dword << 1) | (prn_sequence[sat_offset] ? 0x1 : 0x0);
	write_u64_to_cp_reg (base_addr_u64 + J, dword);


	//
	// write the status word into the command
	// data structure.
	//
	uint64_t status_word = 0;
	status_word = 1;
	status_word = (status_word |  (satellite_id << 8) | (band << 16));
	cp_state->command_response_array[satellite_id].args[7] = status_word;

	return(0);
}

//
// The PRN sequence is stored in the coprocessor in sign-form.
//
int activateSatelliteAndCopyPrnToCoprocessor
		 (CoprocessorState* cp_state,
					uint8_t satellite_id, 
					uint8_t band, 
					int8_t* prn_sequence)
{
	int I;
	int J;

	
	uint64_t dword = 0;

	int satellite_index = satellite_id - 1;
	uint32_t base_address_u32 = 
			(PRN_REGISTERS_ADDR_MIN + (satellite_index * 32));
	uint32_t base_addr_u64 = (base_address_u32 >> 1);


	for(I=0; I < 1023; I++)
	{
		if((I % 64) == 0)
		{
			if(I > 0) 
			{
				J = ((I/64) - 1);
			 	write_u64_to_cp_reg (base_addr_u64 + J, dword);
			}
			dword = 0;
		}

		dword = (dword << 1)  | ((prn_sequence[I] == 1) ? 0x0 : 0x1);
	}

	J = 15;
	// wrap-around: bit 1024 = bit 0.
	dword = (dword << 1) | (prn_sequence[0] ? 0x1 : 0x0);
	write_u64_to_cp_reg (base_addr_u64 + J, dword);


	//
	// write the status word into the command
	// data structure.
	//
	uint64_t status_word = 0;
	status_word = 1;
	status_word = (status_word |  (satellite_index << 8) | (band << 16));
	cp_state->command_response_array[satellite_index].args[7] = status_word;

	return(0);
}

//
// This generates the PRN sequence using the standard
// satellite numbering 
//    1-32 GPS
//    33-46 IRNSS  L5
//    47-60 IRNSS  S
//    61-63 GAGAN  L1
//    64    DUMMY
//
int8_t __prn_buffer__[1024];
int generatePrnAndActivateSatellite(CoprocessorState* cp_state,
					uint8_t satellite_id, 
					uint8_t band)
{
	int status = generatePrnSequence(satellite_id, __prn_buffer__);
	if(status)
	{
		PRINTF("Error: could not generate prn sequence for satellite %d\n", satellite_id);
		return(1);
	}

#ifdef DEBUG_PRINTF
	PRINTF("Info: in generatePrnAndActivateSatellite sat_id=%d, buffer=0x%x\n", satellite_id,
				(uint32_t) &__prn_buffer__);
#endif
	status = activateSatelliteAndCopyPrnToCoprocessor(cp_state, satellite_id, band, __prn_buffer__);
	return(status);
}


//
// deactivate satellite.
//
int deactivateSatellite(CoprocessorState* cp_state, uint8_t satellite_id)
{
	cp_state->command_response_array[satellite_id].args[7] = 0x0;
	return(0);
}

void clearCommandBuffer(CoprocessorState* cp_state, uint8_t command_id)
{
	cp_state->command_response_array[command_id].args[0] = 0;
}

//
// schedule acquire command 
//
// return 0 on success, 1 on failure
int  scheduleAcquireCommand(CoprocessorState* cp_state,
				uint8_t satellite_id,
				// only coherent combining supported.
				uint8_t diff_combine_mode, // 0=no-diff-combin 1=coherent 2=incoherent 3=TBD.
				uint8_t band, // l1=1, l5=2, s=3
				uint8_t num_rf_blocks,
				float  doppler_min,
				float  doppler_max,
				float  doppler_bin_size,
				uint32_t code_delay_min,
				uint32_t code_delay_max,
				uint32_t code_delay_bin_size,
				uint32_t drop_threshold)
{
	CpCommandStatus command_status =  parseCommandResponse(cp_state, satellite_id, NULL, NULL, NULL, NULL);
	if((command_status != INVALID) && (command_status != COMPLETED))
	{
#ifdef DEBUG
		PRINTF("Error: could not schedule acquire command %d. Buffer not free.\n", satellite_id);
#endif
		return(1);
	}

	if((doppler_min > doppler_max) || (doppler_min < 0.0))
	{
		PRINTF("Error: in schedule acquire command  for satellite %d. Illegal doppler range %f, %f.\n", satellite_id, doppler_min, doppler_max);
		return(1);
	}
	if(doppler_bin_size <= 0.0)
	{
		PRINTF("Error: in schedule acquire command  for satellite %d. Illegal doppler bin size %f.\n", satellite_id, doppler_bin_size);
		return(1);
	}
	
	// scheduled valid-cmd cmd-started cmd-finished sequence-id  op-code  reg-id  command-data-value
	//   1           1        1             1         6           4       18           32
	
	// scheduled = 1.
	uint64_t cmd_dword = 1;
	
	//
	//   valid = 0, started = 0, finished = 0
	cmd_dword = (cmd_dword << 3);

	// sequence-id
	cmd_dword = (cmd_dword << 6) | satellite_id;

	// op-code
	cmd_dword = (cmd_dword << 4) | ACQUIRE_OP;

	// no reg-id..
	cmd_dword = (cmd_dword << 50);

	//   drop-threshold  diff-combining  num_rf_blocks satellite-id   band   
	//      18               2              4              6           2	
	uint32_t cmd_data = 
				(drop_threshold << 14) |
				(diff_combine_mode << 12) |
				(num_rf_blocks << 8) |
				(satellite_id << 2) |
				band;
		 
	cmd_dword = cmd_dword | cmd_data;


	//
	// signed values bitcast to uint32.
	//
	uint32_t rescaled_doppler_min = rescaleAcquireToUint32 (cp_state, doppler_min);
	uint32_t n_doppler_bins = ((uint32_t) ((doppler_max - doppler_min)/doppler_bin_size))+1;
	uint32_t rescaled_doppler_bin_size = rescaleAcquireToUint32 (cp_state, doppler_bin_size);
	
	// wrap-around 
	if(code_delay_max < code_delay_min)
		code_delay_max += cp_state->acquire_samples_per_ms;
	uint32_t n_code_delays = ((code_delay_max - code_delay_min)/code_delay_bin_size)+1;
		
	uint32_t total_bins = n_doppler_bins * n_code_delays;
	
	// each pass has at most 512 bins.
	uint32_t number_of_passes = (total_bins >> 9);
	if ((number_of_passes << 9) != total_bins)
		number_of_passes++;

	// calculate best block size..
	uint32_t doppler_block_size = 1;
	uint32_t code_delay_block_size = 1;
	uint32_t count = 1;
	while(1)
	{
#ifdef DEBUG
		PRINTF("setting block sizes %d, %d, %d\n", doppler_block_size, code_delay_block_size, count);
#endif
		count = (count*2);
		if(count <= 512)
		{
			if(code_delay_block_size < n_code_delays)
				code_delay_block_size = (code_delay_block_size << 1);
			else if(doppler_block_size < n_doppler_bins)
				doppler_block_size = (doppler_block_size << 1);
			else
				break;
		}
		else
			break;
	}
#ifdef DEBUG
	PRINTF("Info: acquire block sizes: doppler=%d, code-phase=%d\n", 
					doppler_block_size, code_delay_block_size);
#endif

	// 10 bits.
	doppler_block_size = (doppler_block_size & 0x3ff);
	code_delay_block_size = (code_delay_block_size & 0x3ff);

	// r0 = min-doppler-val
	// r1 = [unused [6-bits] doppler-block-size [10-bits] number-of-doppler-bins [16-bits]]
	// r2 = min-code-offset
	// r3 = [unused [6-bits] code-phase-block-size [10-bits] number-of-code-phase-bins [16-bits]]
	// r4 = doppler-bin-size
	// r5 = code-phase-bin-size
	// r6 = doppler-block-step 
	// r7 = code-phase-block-step.

							
	uint64_t sval = rescaled_doppler_min; // r0
	sval = (sval << 32) | (doppler_block_size << 16) | n_doppler_bins; // r0 r1
	cp_state->command_response_array[satellite_id].args[1] = sval;
#ifdef DEBUG
	PRINTF("Info: acquire arg[1] = 0x%x 0x%x\n",
			(uint32_t) (sval >> 32), (uint32_t) (sval & 0xffffffff));
#endif


	sval = code_delay_min;	// r2
	sval = (sval << 32) | (code_delay_block_size << 16) | n_code_delays; // r2 r3
	cp_state->command_response_array[satellite_id].args[2] = sval;
#ifdef DEBUG
	PRINTF("Info: acquire arg[2] = 0x%x 0x%x\n",
			(uint32_t) (sval >> 32), (uint32_t) (sval & 0xffffffff));
#endif

	sval = rescaled_doppler_bin_size;	 // r4
	sval = (sval << 32) | code_delay_bin_size;			// r4 r5
	cp_state->command_response_array[satellite_id].args[3] = sval;
#ifdef DEBUG
	PRINTF("Info: acquire arg[3] = 0x%x 0x%x\n",
			(uint32_t) (sval >> 32), (uint32_t) (sval & 0xffffffff));
#endif

	// unsigned uint32_t
	sval = rescaleAcquireToUint32(cp_state, doppler_bin_size * doppler_block_size); // r6
	sval = (sval << 32) | (code_delay_bin_size * code_delay_block_size); // r6 r7
	cp_state->command_response_array[satellite_id].args[4] = sval;
#ifdef DEBUG
	PRINTF("Info: acquire arg[4] = 0x%x 0x%x\n",
			(uint32_t) (sval >> 32), (uint32_t) (sval & 0xffffffff));
#endif

	// write the command dword at the end..
	cp_state->command_response_array[satellite_id].args[0] = cmd_dword;
#ifdef DEBUG
	PRINTF("Info: acquire arg[0] = 0x%x 0x%x\n",
			(uint32_t) (cmd_dword >> 32), (uint32_t) (cmd_dword & 0xffffffff));
#endif
	return(0);
}

//
// schedule track command...
//    returns 0 on success.
//
int  scheduleTrackCommand(CoprocessorState* cp_state,
				uint8_t satellite_id,
				uint8_t band,
				float carrier_frequency,
				float carrier_phase,
				uint32_t early_delay,
				uint32_t prompt_delay,
				uint32_t late_delay)
{

	CpCommandStatus command_status =  parseCommandResponse(cp_state, satellite_id,  NULL, NULL, NULL, NULL);

	if((command_status != INVALID) && (command_status != COMPLETED))
	{
#ifdef DEBUG
		PRINTF("Error: could not schedule track command %d. Buffer not free.\n", satellite_id);
#endif
		return(1);
	}

	if(carrier_frequency < 0.0)
	{
		PRINTF("Error: in schedule track command  for satellite %d. Illegal carrier frequency %f.\n", satellite_id, carrier_frequency);
		return(1);
	}

	// The command word is a 64-bit quantity with the following format
	// sched valid-cmd cmd-started cmd-finished sequence-id  op-code  reg-id  command-data-value
	//   1      1          1             1         6           4       18           32
	//


	//   sched = 1
	uint64_t cmd_dword = 1;

	// valid, started, finished are 0
	cmd_dword = (cmd_dword << 3);

	// command-id
	cmd_dword = (cmd_dword << 6) | satellite_id;

	// opcode
	cmd_dword = (cmd_dword << 4) | TRACK_OP;
	cmd_dword = (cmd_dword << 50);

	//  command-data..
	//   unused  satellite-id band 
	//     24      6           2     
	uint32_t cmd_data =  (satellite_id << 2) | band;

	cmd_dword = cmd_dword | cmd_data;

	//
	// signed values bitcast to uint32.
	//
	uint32_t rescaled_carrier_frequency  = 
				rescaleFreqToUint32 (cp_state, carrier_frequency);
	uint32_t rescaled_carrier_phase = rescalePhaseToUint32 (cp_state, carrier_frequency, 
										carrier_phase);
							
	uint64_t sval = rescaled_carrier_frequency;
	sval = (sval << 32) | rescaled_carrier_phase;
	cp_state->command_response_array[satellite_id].args[1] = sval;
#ifdef DEBUG
	PRINTF("Info: track arg[1] = 0x%x 0x%x\n",
			(uint32_t) (sval >> 32), (uint32_t) (sval & 0xffffffff));
#endif

	sval = early_delay;
	sval = (sval << 32) | prompt_delay;
	cp_state->command_response_array[satellite_id].args[2] = sval;
#ifdef DEBUG
	PRINTF("Info: track arg[2] = 0x%x 0x%x\n",
			(uint32_t) (sval >> 32), (uint32_t) (sval & 0xffffffff));
#endif

	sval = late_delay;
	sval = (sval << 32);
	cp_state->command_response_array[satellite_id].args[3] = sval;
#ifdef DEBUG
	PRINTF("Info: track arg[3] = 0x%x 0x%x\n",
			(uint32_t) (sval >> 32), (uint32_t) (sval & 0xffffffff));
#endif


	// command dword at the end.
	cp_state->command_response_array[satellite_id].args[0] = cmd_dword;

	return (0);
}


// return 0 on success.
int scheduleRegisterAccessCommand (CoprocessorState* cp_state,
					uint8_t command_id,
					uint8_t read_write_bar,
					uint8_t band,
					uint32_t reg_id,    // identifies a 32-bit register,  bottom 20 bits used.
					uint32_t reg_value  // 32-bit value.
				)
{
	CpCommandStatus command_status =  parseCommandResponse(cp_state, command_id, NULL, NULL, NULL, NULL);
	if((command_status != INVALID) && (command_status != COMPLETED))
	{
#ifdef DEBUG
		PRINTF("Error: could not schedule register access command %d. Buffer not free.\n", command_id);
#endif
		return(1);
	}


	// The command word is a 64-bit quantity with the following format
	// sched valid-cmd cmd-started cmd-finished sequence-id  op-code  reg-id  command-data-value
	//   1        1        1             1         6           4       18           32
	//
	//   	sched bit
	uint64_t cmd_dword = 1;

 	// valid, started, finished bits initially 0.
	cmd_dword = cmd_dword << 3;

	// sequence id.
	cmd_dword = (cmd_dword << 6) | command_id;	


	uint8_t op_code = (read_write_bar ? READ_REG_OP : WRITE_REG_OP);
	
	// op-code
	cmd_dword = (cmd_dword << 4) | op_code;

	cmd_dword = (cmd_dword << 18);
	cmd_dword = cmd_dword | reg_id;
	cmd_dword = (cmd_dword << 32);

	if(op_code == WRITE_REG_OP)
		cmd_dword = cmd_dword | reg_value;

	
	uint32_t vptr = (uint32_t) &(cp_state->command_response_array[command_id].args[0]);
#ifdef DEBUG
	PRINTF("Info: scheduleRegisterAccess vptr = 0x%x \n", vptr);
#endif


	cp_state->command_response_array[command_id].args[0] = cmd_dword;
#ifdef DEBUG
	PRINTF("Info: scheduled register access command for  id = %d cmd_dword = (0x%x, 0x%x) (va=0x%x)\n", 
				command_id, (uint32_t) (cmd_dword >> 32), (uint32_t) (cmd_dword & 0xffffffff), 
				vptr);
#endif

	return(0);
}



CpCommandStatus parseCommandResponse(CoprocessorState* cp_state,
					uint8_t command_id, 
					// what was the opcode of the command?
					CpCommand* op_code,
					// what was the register value
					uint32_t* register_value,
					// what was the RF block id?
					uint32_t* rf_block_id,
					// up to 8 out args available, 
					// will depend on the op_code.
					uint32_t* out_args)
{
	CpCommandStatus ret_val = INVALID;
	uint64_t rval = cp_state->command_response_array[command_id].args[0];
#ifdef DEBUG
	PRINTF("Info: parsing value (0x%x,0x%x)\n", (uint32_t) (rval >> 32), (uint32_t) (rval & 0xffffffff));
#endif
	uint8_t  top_4 = (rval >> 60);
	if(top_4 & 0x8)
	{
		ret_val = SCHEDULED;
		if(top_4 & 0x1)
			ret_val = COMPLETED;
		else if(top_4 & 0x2)
			ret_val = STARTED;
		else if(top_4 & 0x4)
			ret_val = ACTIVE;
	}
		
	if(ret_val == COMPLETED)
	{
		uint8_t oc = (rval >> 50) & 0xf;
		if(op_code != NULL)
			*op_code = oc & 0xf;

		if(register_value != NULL)
			*register_value = rval & 0xffffffff;

		if(rf_block_id != NULL)
			*rf_block_id = (rval >> 32) & 0x1;

		if((out_args != NULL) && ((oc == TRACK_OP) || (oc  == ACQUIRE_OP)))
		{
			uint64_t arg64 = cp_state->command_response_array[command_id].args[1];
			out_args[0] = 	(arg64 >> 32);
			out_args[1] =   (arg64 & 0xffffffff);


			arg64 = cp_state->command_response_array[command_id].args[2];
			out_args[2] = 	(arg64 >> 32);
			out_args[3] =   (arg64 & 0xffffffff);

			if((oc == TRACK_OP) || (oc == ACQUIRE_OP))
			{
				arg64 = cp_state->command_response_array[command_id].args[3];
				out_args[4] = 	(arg64 >> 32);
				if(oc == TRACK_OP)
				{
					out_args[5] =   (arg64 & 0xffffffff);

					// outgoing phase.
					arg64 = cp_state->command_response_array[command_id].args[4];
					out_args[6] = 	(arg64 >> 32);
				}
			}
		}
	}

	return(ret_val);
}


#define	ONE_NCO_ROTATION   (2.0*(1u<<31))

const double tb_inv_2pi = 1.0/(2.0*M_PI);
const double tb_2pi = 2.0*M_PI;
const uint32_t  c_tb_tblsize   = NCO_SINE_SIGN_TABLE_SIZE; 
const double one_nco_rotation = ONE_NCO_ROTATION;

uint32_t rescaleFreqToUint32 (CoprocessorState* cp_state, double freq)
{
	double fratio = (freq/cp_state->internal_sampling_frequency);
	double rescaled_double_val = (1.0*fratio*one_nco_rotation);
	uint32_t ret_val = (uint32_t) rescaled_double_val;
	return(ret_val);
}

double   rescaleFreqFromUint32(CoprocessorState* cp_state, uint32_t rescaled_freq)
{
	double dret_val = (((double) rescaled_freq)*cp_state->internal_sampling_frequency)/ONE_NCO_ROTATION;
	return(dret_val);
}

uint32_t rescalePhaseToUint32 (CoprocessorState* cp_state, double frequency,  double phase)
{
	int R;

	//
	// we dont like negative numbers.
	// 
	if(phase < 0.0)
	{
		double nphase = -phase;

		// ratio of phase to 2pi.
		R =  (nphase*tb_inv_2pi);

		double offset_phase = tb_2pi*R;
		if(offset_phase < nphase)
			offset_phase += tb_2pi;

		// translate by a multiple of 2pi and
		//  make phase positive.
		phase = offset_phase + phase;
	}

	// keep phase between 0 and 2pi.
	R = (tb_inv_2pi*phase);
	phase = phase - (tb_2pi*R);
	
	//
	// phase = 2pi corresponds to one rotation
	//
	double fratio = (tb_inv_2pi*phase);
	double rescaled_double_val = (fratio*one_nco_rotation);

	uint32_t ret_val = (uint32_t) rescaled_double_val;
	return(ret_val);
}

double   rescalePhaseFromUint32(CoprocessorState* cp_state, uint32_t rescaled_phase)
{
	double dret_val = (((double) rescaled_phase)*2.0*M_PI*cp_state->internal_sampling_frequency)/ONE_NCO_ROTATION;
	return(dret_val);
}

uint32_t rescaleAcquireToUint32 (CoprocessorState* cp_state, double freq)
{
	double fratio = (freq/cp_state->internal_acquire_sampling_frequency);
	double rescaled_double_val = (1.0*fratio*one_nco_rotation);
	uint32_t ret_val = (uint32_t) rescaled_double_val;
	return(ret_val);
}

double   rescaleAcquireFromUint32(CoprocessorState* cp_state, uint32_t rescaled_freq)
{
	double dret_val = (((double) rescaled_freq)*cp_state->internal_acquire_sampling_frequency)/ONE_NCO_ROTATION;
	return(dret_val);
}

int32_t coprocessorAddAcquisitionCodePhases(CoprocessorState* cp_state,
						int32_t cphase, int32_t increment)
{
	int32_t result = cphase + increment;
	if(result < 0)
	{
		result = result + cp_state->acquire_samples_per_ms;
	}
	else if(result > cp_state->acquire_samples_per_ms)
	{
		result = result - cp_state->acquire_samples_per_ms;
	}
	return(result);
}

int32_t coprocessorAddTrackingCodePhases(CoprocessorState* cp_state,
						int32_t cphase, int32_t increment)
{
	int32_t result = cphase + increment;
	if(result < 0)
	{
		result = result + cp_state->track_samples_per_ms;
	}
	else if(result > cp_state->track_samples_per_ms)
	{
		result = result - cp_state->track_samples_per_ms;
	}
	return(result);
}


double calculateAcquireFom(int bin_count, uint32_t peak, uint32_t scaled_square_sum)
{
	double ret_val  = 0.0;
	if(bin_count > 0)
	{
		double max_e = peak;
		double rms_scaled_e = scaled_square_sum;
		ret_val = (max_e * bin_count) / (256.0 * rms_scaled_e);
	}
	return(ret_val);
}




void coprocessor_handler ()
{
	//
	// check the PVT side configuration information
	// and mark the satellite status accordingly.
	//
	//
	//probeCcr (ccr, csr);

	//
	// sweep across all satellites, and analyze responses
	//
	//analyzeCpResponses (csr);	

	// 
	// generate commands for the next ms.
	//
	//generateCpCommands (csr);


	return;
}


