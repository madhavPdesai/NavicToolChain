#ifndef ___isr_h___
#define ___isr_h___

#include <math.h>
#include <navic_includes.h>
extern NavicState navic_state;


//  Functions to be provided by users of the coprocessor. These functions
//  are used to tranfer information from the coprocessor interrupt service
//  routine to the main application.


//
// The function is provided the IP,QP values for
// the early, prompt and late code delays, as calculated
// by the coprocessor correlator.
//
// The function returns 1 if track is to be continued.
// If it returns 1, then it specifies the track parameters
// for the next iteration in the pass-by-reference
// arguments.
//
// Functionality:
//    takes the ip, qp values and updates the tracking
//    loop. indicates whether tracking should go on,
//    and if so, the new parameters.
//
// 
int updateTrackInformation (	uint8_t satellite_id,
				uint32_t early_ip, uint32_t early_qp,
				uint32_t prompt_ip, uint32_t prompt_qp,
				uint32_t late_ip, uint32_t late_qp,
				uint32_t residual_phase,
				//
				// values for next iteration.
				//
				uint8_t* band, // for L1 band=1, for L5 band=2, for S band=3
				float*   carrier_frequency,
				float*   carrier_phase,
				uint32_t* early_code_delay,
				uint32_t* prompt_code_delay,
				uint32_t* late_code_delay);

//
// update acquisition information at higher level.
//
// Functionality: records the fact that all acquire
//    correlations for the satellite have been completed,
//    and reports the best power etc.
//
void updateAcquireInformation (	uint8_t satellite_id,
				uint32_t best_power, 
				uint32_t best_doppler,
				uint32_t best_code_phase);
				
				
//
// read register response.
// Functionality
//    records the register value which was requested.
//
void updateReadRegInformation (uint8_t satellite_id, uint32_t register_value);


//
// indicate completion of read register command.
// Functionality
//    records the completion of a NOP/write-reg command.
//
void updateWriteRegOrNopInformation (uint8_t satellite_id);
					

#endif
