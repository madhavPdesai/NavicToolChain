#ifndef cp_driver_h___
#define cp_driver_h___

#include <cp_rf_parameters.h>


//////////////////////////////////////////////////////////////////////////////////
// These defines should not be modified!					//
//////////////////////////////////////////////////////////////////////////////////
// Internal sampling frequency in the NAVIC SOC.
#define CP_RF_INTERNAL_SAMPLING_FREQUENCY		(1.023 * 16.0 * 1000000.0)
#define CP_CONTROL_REGISTER_ID				0
#define CP_SHARED_MEMORY_COMMAND_REGN_BASE_REGISTER_ID	1
#define CP_SHARED_MEMORY_MAX_COMMAND_COUNT_REGISTER_ID	2
#define CP_INTERRUPT_INTERVAL_REGISTER_ID		3
#define CP_INTERRUPT_PHASE_1_INTERVAL_REGISTER_ID	4

#define CP_ENABLE_MASK 					0x1	 // bit 0
#define CP_INTR_ENABLE_MASK				0x2	 // bit 1
#define CP_SET_INTR_MASK				0x4	 // bit 2
#define CP_CLEAR_INTR_MASK				0x8	 // bit 3
#define CP_COMMAND_STROBE_MASK				0x10	 // bit 4
#define CP_TOGGLE_RF_BUFFER_ID_MASK			(1 << 5) // bit 5
#define CP_TOGGLE_RF_L1_ENABLE_MASK			(1 << 6) // bit 6
#define CP_TOGGLE_RF_L5_ENABLE_MASK			(1 << 7) // bit 7
#define CP_TOGGLE_RF_S_ENABLE_MASK			(1 << 8) // bit 8
#define CP_RF_START_MASK				(1 << 9) // bit 9

// virtual base address of cp command buffer page
#define CP_COMMAND_BUFFER_VIRTUAL_ADDR_BASE		0x40064000
//
// Located the command buffer at PA mapped to internal
// RAM.
//
#define CP_COMMAND_BUFFER_PHYSICAL_ADDR_BASE		0x40050000


//////////////////////////////////////////////////////////////////////////////////

void setCpLinkVerboseFlag(int v);

void writeCoprocessorControlWord(uint32_t control_mask);
uint32_t readCoprocessorControlWord();

int toggleTest();
void toggleActiveRfBlockId();

//  index of rf word register.
//     (return 0x0 if not legal).
uint32_t get_rf_reg_id (uint8_t acq_flag, uint8_t band, uint8_t block_id, uint32_t offset);

// write to specified offset in block for either acquire data or track data.
void writeCoprocessorRfWord(uint8_t acq_flag,
				uint8_t band, uint8_t block_id, uint32_t offset, uint32_t rf_word);
// read from specified offset in block for either acquire data or track data.
uint32_t readCoprocessorRfWord(uint8_t acq_flag,
				uint8_t band, uint8_t block_id, uint32_t offset);
void clearAcquireRfBuffers(uint8_t rf_band);
void clearTrackRfBuffers(uint8_t rf_band);

// clear interrupt.
void clearInterrupt();

// set max ms download counts.. use for setting a limit
// on the number of ms that will be downloaded.
// If set to 0, the RF mem interface will run freely.
// Else it will stop after downloading the specified number
// of ms.
void setRfMaxMsCount(uint8_t l1_max, uint8_t l5_max, uint8_t s_max);

// Get the ms counts obtained from the three RF interfaces.
void getRfMsCounts (uint32_t* l1_count, uint32_t* l5_count, uint32_t* s_count);
void setRfMsCount(uint8_t rf_band, uint32_t count);



//
// schedule command and track command status probes.. will probe 
// the current situation in the acquire/track execution.
//
void probeAcquireCommandStatus(uint32_t* cell_count, uint8_t* command_id,  uint8_t* started, uint8_t *finished, uint8_t* waiting_for_rf);
void probeTrackCommandStatus(uint8_t* command_id,  uint8_t* started, uint8_t *finished);

//
// Initialize the coprocessor
//    1. set cp_state->command_response_array pointer = pointer_to_command_array
//    2. write max-number-of-commands, interrupt-interval, phase-1-duration to coprocessor
//          (also internal sampling frequency)
//
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
				uint32_t phase_1_duration
				);

//
// write to coprocessor control word, interrupt flag is
// written into control word.
//
// Note:  after the coprocessor is enabled:
//   1. it will start reading from the L1,L5,S interfaces
//      into the respective rf buffers.
//   2. the coprocessor will raise an interrupt every millisecond.
//   3. phase_1_duration after the interrupt, the coprocessor will start
//       polling and executing commands in the command array.
//       
//       
void enableCoprocessor(CoprocessorState* cp_state, uint8_t interrupt_flag);


// if you wish to independently set or get the intervals.
void setCoprocessorInterruptIntervals(CoprocessorState* cp_state,
						uint32_t interrupt_interval, uint32_t phase_1_duration);
// values returned by reference.
void getCoprocessorInterruptIntervals(CoprocessorState* cp_state,
						uint32_t *interrupt_interval, uint32_t* phase_1_duration);


// Adc clock can be set to either rising edge (arg=0) or falling ege (arg=1)
void setCoprocessorAdcClocking (uint8_t l1_falling_edge, uint8_t l5_falling_edge, uint8_t s_falling_edge);
void getCoprocessorAdcClocking (uint8_t* l1_falling_edge, uint8_t* l5_falling_edge, uint8_t* s_falling_edge);

CpCommandDaemonStatus getCoprocessorCommandDaemonStatus(CoprocessorState* cp_state);

// old version.
int activateSatellite (CoprocessorState* cp_state,
					uint8_t satellite_id, 
					uint8_t band, 
					int8_t* prn_sequence);
//
// activation of satellite.
//   activate the satellite.
//
//   if prn_sequence is not NULL, write it into
//   the coprocessor.
//      The prn-sequence is an array of 1023 bytes, with
//      the '1' value being taken as  1 and '0' value
//      being taken as -1
//
//  Notes to self: the 1024th bit will be the same as
//   the first bit, when written to the CP.
//
int activateSatelliteAndCopyPrnToCoprocessor 
		(CoprocessorState* cp_state,
					uint8_t satellite_id, 
					uint8_t band, 
					int8_t* prn_sequence);

//
// This generates the PRN sequence using the standard
// satellite numbering 
//    1-32 GPS
//    33-46 IRNSS  L5
//    47-60 IRNSS  S
//    61-63 GAGAN  L1
//    64    DUMMY
//
int generatePrnAndActivateSatellite(CoprocessorState* cp_state,
					uint8_t satellite_id, 
					uint8_t band);

int deactivateSatellite(CoprocessorState* cp_state, uint8_t satellite_id);
SatelliteStatus getSatelliteStatus (CoprocessorState* cp_state, uint8_t satellite_id);


//
// clears the command buffer control dword.
// 
void clearCommandBuffer(CoprocessorState* cp_state, uint8_t command_id);

// This command schedules an acquire command at the next interrupt.
//   The satellite must be activated.
//
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
				uint32_t drop_threshold);
//
// This command schedules a track command at the next interrupt.
//
int  scheduleTrackCommand(CoprocessorState* cp_state,
				uint8_t satellite_id,
				uint8_t band,
				float carrier_frequency,
				float carrier_phase,
				uint32_t early_delay,
				uint32_t middle_delay,
				uint32_t late_delay);

//
// other commands: nop, write-reg, read-reg
//   return 0 on success.
//
int scheduleRegisterAccessCommand (CoprocessorState* cp_state,
					uint8_t command_id,
					uint8_t read_write_bar,
					uint8_t band,
					uint32_t reg_id,    // identifies a 32-bit register,  bottom 18 bits used.
					uint32_t reg_value  // 32-bit value.
				  );



//
//
// top-4-bits can have following values
//  scheduled valid started finished
//   0         _        _       _     invalid
//   1         0        _       _     scheduled
//   1         1        0       0     valid command
//   1         1        1       0     valid command, started.
//   1         1        1       1     valid command, finished.
//
CpCommandStatus parseCommandResponse(CoprocessorState* cp_state,
					uint8_t command_id, 
					// what was the opcode of the command?
					CpCommand* op_code,
					// what was the register value?
					uint32_t* register_value,
					// block id used for track correlations
					uint32_t* rf_block_id, 
					// up to 8 out args available, 
					// will depend on the op_code.
					uint32_t* out_args);

// return response dword.
uint64_t getResponseU64(CoprocessorState* cp_state, 
				uint8_t command_id, uint8_t dword_id);

// Get a 32-bit word with index word-id from the command area.
uint64_t getResponseU32(CoprocessorState* cp_state, 
				uint8_t command_id, uint8_t word_id);

uint32_t rescaleFreqToUint32 (CoprocessorState* cp_state, double freq);
double   rescaleFreqFromUint32(CoprocessorState* cp_state, uint32_t rescaled_freq);

uint32_t rescalePhaseToUint32 (CoprocessorState* cp_state, double frequency, double phase);
double   rescaleFreqFromUint32(CoprocessorState* cp_state, uint32_t rescaled_phase);

uint32_t rescaleAcquireToUint32 (CoprocessorState* cp_state, double freq);
double   rescaleAcquireFromUint32(CoprocessorState* cp_state, uint32_t rescaled_freq);

void coprocessor_interrupt_handler ();

int32_t coprocessorAddAcquisitionCodePhases(CoprocessorState* cp_state, 
						int32_t cphase, int32_t increment);
int32_t coprocessorAddTrackingCodePhases(CoprocessorState* cp_state, 
						int32_t cphase, int32_t increment);

double calculateAcquireFom(int bin_count, uint32_t peak, uint32_t scaled_square_sum);
#endif
