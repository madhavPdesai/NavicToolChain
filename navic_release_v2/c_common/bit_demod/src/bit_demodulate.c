#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#ifdef AJIT
#include <ajit_access_routines.h>
#endif
//#include "macros.h"
#include "bit_demodulate.h"

// utility
void resetForNextBit(BitDemodFsmState* bdfs, 
				double qp, 
				double doppler, 
				uint32_t code_phase,
				uint32_t ms_count)
{
	bdfs->qp_sum    = qp;
	bdfs->internal_counter = 1;

	bdfs->initial_doppler = doppler;
	bdfs->initial_code_phase = code_phase;
	bdfs->doppler_sum = doppler;

	bdfs->last_code_phase = code_phase;
	bdfs->delta_code_phase = 0;

	bdfs->initial_ms_count = ms_count;
	bdfs->zero_crossings_in_bit_period = 0;
}
						
// code phase is a number between 0 and 16367 and can wrap around
int32_t calculateDeltaCodePhase (uint32_t last_code_phase, uint32_t code_phase)
{
	int32_t ret_val = ((int32_t) code_phase) - ((int32_t) last_code_phase);
	
	// may be code_phase has wrapped around?
	if(ret_val < 0)
	{
		if((-ret_val) > (16368/2))
		// if the difference is huge..
		{
			// wrap around.
			ret_val = ret_val + 16368;
		}
	}
	return(ret_val);
}


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

 */

void initBitDemodFsmState (BitDemodFsmState* bdfs, uint8_t sat_id)
{
	bdfs->fsm_state = RESET;
	bdfs->internal_counter = 0;
	bdfs->scratch_value = 0;
	bdfs->initial_ms_count = 0;
	bdfs->last_ms_count = 0;

	bdfs->satellite_id = sat_id;
#ifdef DUMP_BITS
	bdfs->bit_vector = 0;
	bdfs->bit_counter = 0;
#endif
	bdfs->last_qp = 0.0;
	bdfs->qp_sum = 0.0;

	bdfs->last_code_phase  = 0;
	bdfs->delta_code_phase = 0;

	bdfs->zero_crossings_in_bit_period = 0;
	bdfs->doppler_sum = 0;
}

uint32_t injestQp (BitDemodFsmState* bdfs, 
		// input arguments
		uint32_t ms_count, 
		double qp, 
		uint32_t code_phase,
		double doppler,
		// outputs by reference.
		uint8_t* b,			// bit value
		uint32_t* b_ms_count,		// ms-count at start of bit
		uint32_t* b_code_phase, 	// code-phase at the start of the bit.
		double*   average_code_phase,	// code phase averaged over bit.
		double*   b_doppler,		// doppler at the start of bit.
		double*   average_doppler,	// average doppler across the bit.
		uint8_t*  zero_crossings_in_bit_period,
		double*   qp_average
		)
{
	uint32_t ret_val = 1;
	int bit_val = 0;


	if((bdfs->last_ms_count != 0) && (ms_count != (bdfs->last_ms_count+1)))
	{
#ifdef AJIT
		ee_printf("Error: missed a ms: %d->%d in injestQp for sat id %d\n", 
				bdfs->last_ms_count, ms_count, bdfs->satellite_id);
#else
		fprintf(stderr, "Error: missed a ms: %d->%d in injestQp for sat id %d\n", 
				bdfs->last_ms_count, ms_count, bdfs->satellite_id);
#endif
	}

	//
	// Update the last_ms_count!  We should not be missing samples!
	//
	bdfs->last_ms_count = ms_count;

	switch(bdfs->fsm_state) {
		case RESET:
			bdfs->fsm_state = INITIAL_WAIT;
			bdfs->internal_counter = 1;
			break;
		case INITIAL_WAIT:
			if(bdfs->internal_counter == INITIAL_OFFSET)
			{
				bdfs->fsm_state = WAIT_FOR_TRANSITION;
				bdfs->internal_counter = 1;
			}
			else
				bdfs->internal_counter++;
			break;
		case WAIT_FOR_TRANSITION:
			if(((qp > 0) & (bdfs->last_qp < 0)) ||
					(qp < 0) & (bdfs->last_qp > 0))
			{
				bdfs->internal_counter = 1;

				if(bdfs->scratch_value == 0)
				{
					bdfs->initial_ms_count = ms_count;
				}

				bdfs->scratch_value++;

				if((bdfs->scratch_value > 1) &&
						(((ms_count - bdfs->initial_ms_count)%20) != 0))
				{
					bdfs->scratch_value = 0;
					bdfs->initial_ms_count = ms_count;
				}
				// after 3 successful bit changes with
				// x20ms intervals, declare victory.
				else if(bdfs->scratch_value == BITSYNCHTHRESHOLD)
				{
					bdfs->fsm_state = DECIDEBITS;
					resetForNextBit(bdfs, qp, doppler, code_phase, ms_count);	
				}
			}
			break;
		case DECIDEBITS:
			bit_val = (bdfs->qp_sum > 0.0) ? 1 : 0;
	
			//
			// count zero crossings in bit period.
			//
			if( ((qp > 0) && (bdfs->last_qp < 0)) ||
					((qp < 0) && (bdfs->last_qp > 0)))
			{
				bdfs->zero_crossings_in_bit_period++;
			}
			if(bdfs->internal_counter == 20)
			{

				*zero_crossings_in_bit_period = bdfs->zero_crossings_in_bit_period;
				bdfs->zero_crossings_in_bit_period = 0;
				*qp_average = (0.05 * bdfs->qp_sum);
				*b_doppler = bdfs->initial_doppler;
				*average_doppler = (0.05 * bdfs->doppler_sum);

				*b_code_phase = bdfs->initial_code_phase;
				*average_code_phase =  bdfs->initial_code_phase +
								(0.05 * bdfs->delta_code_phase);

				*b_ms_count = bdfs->initial_ms_count;

				*b = bit_val;
				ret_val = 0;

			
#ifdef OFFLINE_PRINT
#ifdef AJIT
				ee_printf("MPD sat=%d ms_period = [%d:%d] qp_sum=%lf bit_val=%d\n", 
						bdfs->satellite_id,
						bdfs->initial_ms_count,
						ms_count-1,
						bdfs->qp_sum,
						bit_val); 
#else
				fprintf(stderr,
					"MPD sat=%d ms_period = [%d:%d] qp_sum=%lf bit_val=%d\n", 
						bdfs->satellite_id,
						bdfs->initial_ms_count,
						ms_count-1,
						bdfs->qp_sum,
						bit_val); 
#endif
#endif

				// reset for bext bit.
				resetForNextBit(bdfs, qp, doppler, code_phase, ms_count);	
			}
			else
			{
				bdfs->qp_sum += qp;
				bdfs->doppler_sum += doppler;
				int32_t dcp = 
					calculateDeltaCodePhase (bdfs->last_code_phase, code_phase);

				bdfs->delta_code_phase += dcp;

				bdfs->internal_counter++;
			}
			break;
		default:
			break;
	}	
			
	bdfs->last_qp = qp;
	bdfs->last_code_phase = code_phase;


#ifdef DUMP_BITS
	if(ret_val == 0)
	{
		bdfs->bit_vector = (bdfs->bit_vector << 1) | bit_val;
		bdfs->bit_counter++;

		if(bdfs->bit_counter == 32)
		{
#ifdef AJIT
			ee_printf("MPD P0 %d 0x%x\n", bdfs->satellite_id, bdfs->bit_vector);
#else
			fprintf(stderr, "MPD P0 %d 0x%x\n", bdfs->satellite_id, bdfs->bit_vector);
#endif
			bdfs->bit_counter = 0;
			bdfs->bit_vector  = 0;
		}
	}
#endif

	return(ret_val);
}

