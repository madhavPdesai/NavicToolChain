#ifndef bit_demodulate_mpd__h___
#define bit_demodulate_mpd__h___

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define INITIAL_OFFSET 	  	 100
#define BITSYNCHTHRESHOLD 	 4

/*
	bit demodulator based on QP values.
	
	starts in an initial wait state (lets
	100ms pass).  
	
	then waits for a + to - or - to + transition
	and starts recording bits.  If there are
	20 consecutive +'s then it emits a 1 else
	it emits a 0.

	Note: this simple state machine does not
	handle the case when there are multiple
	Qp transitions within a single 20ms interval.

	collects 8 bits

*/

typedef enum _FsmState {
	RESET,
	INITIAL_WAIT,
	WAIT_FOR_TRANSITION,
	DECIDEBITS
} FsmState;

typedef struct __BitDemodFsmState {

	// internal values.

	// ms at beginning of bit.
	uint32_t initial_ms_count;
	uint32_t last_ms_count;

	uint32_t internal_counter;
	uint32_t scratch_value;

	// used for determining bit.
	double  last_qp;
	double  qp_sum;

	// doppler at beginning of bit.
	double  initial_doppler;

	double  doppler_sum;

	uint32_t initial_code_phase;
	uint32_t last_code_phase;
	int32_t  delta_code_phase;

#ifdef DUMP_BITS
	// collect and print..
	uint32_t bit_vector;
	uint32_t bit_counter;
#endif
	

	uint8_t  satellite_id;
	uint8_t  zero_crossings_in_bit_period;
	FsmState fsm_state;

} BitDemodFsmState;

void initBitDemodFsmState (BitDemodFsmState* bdfs, uint8_t sat_id);

//
// return 1 on bit-boundary, bit value in uint8_t* b
// returns 0 if no bit is found as yet.
//
uint32_t injestQp (BitDemodFsmState* bdfs, 
			// inputs by value
			uint32_t ms_count,  	// ms index for Qp
			double qp, 		// qp value
			uint32_t code_phase,	// tracking code-phase 
			double doppler,		// tracking doppler
			// outputs by reference.
			uint8_t* b,			// bit value
			uint32_t* b_ms_count,		// ms-count at start of bit
			uint32_t* b_code_phase, 	// code-phase at the start of the bit.
			double*   average_code_phase,	// code phase averaged over bit.
			double*   b_doppler,		// doppler at the start of the bit.
			double*   average_doppler,	// average doppler across the bit.

			//
			// This is an indicator of instability (poor track)
			// and should normally be 0 or 1.
			//
			// zero-crossings inside bit period.
			uint8_t*  zero_crossings_in_bit_period,
				
			//
			// average value of qp during bit period.
			//    average value of qp should be approximately
			//    equal to the qp values fed into the bit demodulator.
			//    small values indicate a problem.
			//
			double*   qp_average
	     );

#endif
