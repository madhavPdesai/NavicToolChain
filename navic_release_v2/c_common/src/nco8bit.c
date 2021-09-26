#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cp_rf_parameters.h>
#include <nco8bit.h>
#include <macros.h>

const int 	c_8_lgtblsize = RF_LOG_COS_TABLE_SIZE;  
const uint32_t  c_8_tblsize   = RF_COS_TABLE_SIZE; 
const uint32_t  c_8_tblmask   = RF_COS_TABLE_MASK;

const uint32_t  c_8_fixed_point_top_bit_index   = 32; // 33 bit (including .) fixed point


#define	ONE_NCO_ROTATION   (2.0*(1u<<31))

void  initNco8bitState (Nco8bitState* nco_state, 
				double sampling_frequency, 
				double signal_frequency,
				double initial_phase)
{
	double one_nco_rotation = ONE_NCO_ROTATION;
	double     inv_2pi = 1.0/ (2.0*M_PI); // 1/(2*pi)

	nco_state->phase_step = (uint32_t) (signal_frequency*one_nco_rotation/sampling_frequency);
	nco_state->current_phase  = (uint32_t) (initial_phase*inv_2pi*one_nco_rotation);

	// initialize cos-table.
	int I; 
	for(I=0;I < RF_COS_TABLE_SIZE/2; I++)
	{
		double phase   = (2*M_PI*I/RF_COS_TABLE_SIZE);
		double cval = cos(phase);
		int8_t abs_cval_fixed_point = (int) (((1 << 7)-1)*cval);
		nco_state->cos_table[I] = abs_cval_fixed_point;
	}	
}

int8_t nextNco8bitOutput (Nco8bitState* nco_state)
{
	// PRINTF("nextNcoOutput: current-phase = 0x%x\n", nco_state->current_phase);

	// Total phase is viewed as a 33-bit fixed point number.
	// throw away the decimal..  (bottom (32-c_8_lgtblsize)  bits)
	//
	//  In this case, the bottom (32-9)=23  bits.
	//
	uint32_t index = nco_state->current_phase >> (c_8_fixed_point_top_bit_index - c_8_lgtblsize);

	// Then,  keep the index modulo table size.
	index = index & c_8_tblmask;
	
	//
	// now increment the accumulated phase.
	//
	nco_state->current_phase = nco_state->current_phase + nco_state->phase_step; 

	// lookup the sine table.
	int8_t retval =  ((index < RF_COS_TABLE_SIZE/2) ? nco_state->cos_table[index]
					: - nco_state->cos_table[index - (RF_COS_TABLE_SIZE/2)]);

	//PRINTF("nco8bit: index=%d, retval=%d\n", index, retval);
	return(retval);
}


