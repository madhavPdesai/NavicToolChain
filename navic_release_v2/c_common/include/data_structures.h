#ifndef __data_structures____
#define __data_structures____

//////////////////////////////////////////////////////////////////////////////////////////
//  Coprocessor										//
//////////////////////////////////////////////////////////////////////////////////////////
typedef enum __SatelliteStatus {
	INACTIVE,
	NOT_ACQUIRED,
	ACQUIRED,
	BEING_TRACKED,
	TRACK_LOST
} SatelliteStatus;

typedef enum __CpCommand {
	NOP_OP=1,
	ACQUIRE_OP=2,
	TRACK_OP=3,
	WRITE_REG_OP=4,
	READ_REG_OP=5
} CpCommand;

typedef enum __CpCommandStatus {
	INVALID,
	SCHEDULED,
	ACTIVE,
	STARTED,
	COMPLETED
} CpCommandStatus;

typedef enum __CpCommandDaemonStatus {
	UNDEFINED	  = 0,
	STARTED_SWEEP	  = 1,
	COMPLETED_SWEEP   = 2
} CpCommandDaemonStatus;

typedef struct __CpCommandResponseRecord CpCommandResponseRecord;
struct __CpCommandResponseRecord {
	//
	// 8 64-bit numbers.
	//  See the documentation.
	// argv[0] is the command dword.
	// argv[1] keeps arguments 1,2
	// argv[2] keeps arguments 3,4
	// argv[3] keeps arguments 5,6
	// argv[4] keeps arguments 7,8
	// argv[5-6] are unused for now.
	// argv[7] keeps a satellite control/status word.
	//   	[63:20] unused
	//	[19:16] band
	//      [15:8]  satellite-id
	//	[7:0]   status
    	//	the status bit fields are
    	//	[0]   active
    	//	[1]   not acquired
    	//	[2]   acquired
    	//	[3]   being tracked
    	//	[4]   lost track
	//
	uint64_t  args[8];
};


typedef struct __CoprocessorState {

	//
	// CpCommandResponseRecord command_response_array[NUMBER_OF_SATELLITES];
	//
	//  Initialize this to a non-cacheable memory location which
	//  is aligned to a page boundary.
	CpCommandResponseRecord* command_response_array;
	uint32_t physical_address_of_command_response_array;

	// index of the coprocessor..
	//   if you want to use multiple coprocessors in the future :-)
	uint32_t coprocessor_index;


	// maximum number of commands..
	uint32_t max_number_of_commands;

	//
	// initialized to 0, increment on every
	// interrupt.
	//
	uint64_t interrupt_counter;

	// internal sampling frequency
	double internal_sampling_frequency;
	int32_t track_samples_per_ms;

	// internal_sampling_frequency/4.
	double internal_acquire_sampling_frequency;
	int32_t acquire_samples_per_ms;

	//
	// interrupt intervals.
	// 
	uint32_t interrupt_interval;
	uint32_t phase_1_duration;


} CoprocessorState;

//////////////////////////////////////////////////////////////////////////////////////////
//  RF resampler									//
//////////////////////////////////////////////////////////////////////////////////////////
typedef struct RfResamplerEngineSnapshot__ {
	uint64_t number_of_samples_received_from_adc;
	uint64_t number_of_samples_transmitted_to_memory;
	uint16_t last_8_samples_from_input_after_upsampler;
	uint16_t last_8_samples_at_output_of_resampler;
} RfResamplerEngineSnapshot;

typedef struct RfResamplerSnapshot__ {
	uint32_t controller_ctrl_register;

	RfResamplerEngineSnapshot l1_engine_snapshot;
	RfResamplerEngineSnapshot l5_engine_snapshot;
	RfResamplerEngineSnapshot s_engine_snapshot;

} RfResamplerSnapshot;

typedef struct RfResamplerEngineConfiguration__ {

	uint8_t  ignore_flag;
	uint8_t  band;
	uint8_t  wipeoff_flag; 
	uint8_t  input_upsample_factor;
	uint64_t dphase_in;
	uint64_t dphase_out;

	uint32_t wipeoff_nco_phase_step;
	uint32_t wipeoff_nco_zero_if_phase_step;

	uint16_t i_val;
	uint16_t number_of_samples_per_ms_at_resampler_input;

	int8_t*  filter_1_coefficients;
	int8_t*  filter_2_coefficients;
	int8_t*  filter_3_coefficients;

} RfResamplerEngineConfiguration;

typedef struct RfResamplerConfiguration__ {

	RfResamplerEngineConfiguration l1_configuration;
	RfResamplerEngineConfiguration l5_configuration;
	RfResamplerEngineConfiguration s_configuration;

	// keep separate for performance reasons.
	int8_t cos_table[RF_COS_TABLE_SIZE];
	int8_t sine_table[RF_COS_TABLE_SIZE];

} RfResamplerConfiguration;

typedef struct RfResamplerState__ {
	RfResamplerConfiguration configuration;
	RfResamplerSnapshot snapshot;
} RfResamplerState;

//////////////////////////////////////////////////////////////////////////////////////////
//  NAVIC										//
//////////////////////////////////////////////////////////////////////////////////////////

typedef struct __NavicState {
	
	CoprocessorState coprocessor_state;
	RfResamplerState resampler_state;
	
} NavicState;

#endif
