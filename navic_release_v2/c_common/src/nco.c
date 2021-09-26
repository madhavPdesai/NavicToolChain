#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cp_rf_parameters.h>
#include <nco.h>

//
// NCO generator code.
//
//#define INTERNAL_SAMPLING_FREQUENCY  16*1023*1000
// table has 16*1023 entries,  log of this is 14.
const int 	c_lgtblsize = NCO_LOG_SINE_SIGN_TABLE_SIZE;  // 14.
const uint32_t  c_tblsize   = NCO_SINE_SIGN_TABLE_SIZE; 
const uint32_t  c_tblmask   = NCO_SINE_SIGN_TABLE_MASK;
const uint32_t  c_z_threshold = NCO_SINE_SIGN_TABLE_I0;
const uint32_t  c_fixed_point_top_bit_index   = 32; // 33 bit (including .) fixed point

// internal sampling rate.
const double	SAMPLE_RATE= INTERNAL_SAMPLING_FREQUENCY;   


//
// These are self explanatory.
//
const double     pi = 3.141592654;	
const double     inv_2pi = 0.159090909; // 1/(2*pi)


int8_t sign_sin(uint32_t k, uint32_t maxlen);

uint32_t nco_dphase;
uint32_t nco_phase;


void  initNcoState (NcoState* nco_state, uint32_t initial_phase, uint32_t phase_step)
{
	nco_state->current_phase = initial_phase;
	nco_state->phase_step = phase_step;
}

// 2-bit sign magnitude..
uint8_t signOfSine (uint32_t idx)
{
	idx = idx & c_tblmask;

	uint8_t ret_val = 1;
#ifdef NTLABS

	uint8_t sign = (idx >= (c_tblsize >> 1));
	uint8_t magn = 1;
	if((idx <= c_z_threshold) || 
		(idx >= (c_tblsize - c_z_threshold)) || 
		((idx >= ((c_tblsize >> 1) - c_z_threshold)) &&
			(idx <= ((c_tblsize  >> 1) + c_z_threshold))))
		magn = 0;
	ret_val = (sign << 1) | magn;

#else
	if((idx <= c_z_threshold) || 
		(idx >= (c_tblsize - c_z_threshold)) || 
		((idx >= ((c_tblsize >> 1) - c_z_threshold)) &&
			(idx <= ((c_tblsize  >> 1) + c_z_threshold))))
	{
		ret_val = 0;
	}
	else
	{
		if(idx  > (c_tblsize >> 1))
		{
			ret_val = 3;
		}
	}
#endif

	return(ret_val);
}


//
// return 1 if negative, 0 if positive.
//
//     Returns Sin( omega t  + phi)
//     and increments t.
//
uint8_t nextNcoOutput (NcoState* nco_state)
{
	// fprintf(stderr,"nextNcoOutput: current-phase = 0x%x\n", nco_state->current_phase);

	// Total phase is viewed as a 33-bit fixed point number.
	// throw away the decimal..  (bottom (32-c_lgtblsize)  bits)
	//
	//  In this case, the bottom 18 bits.
	//
	uint32_t index = nco_state->current_phase >> (c_fixed_point_top_bit_index - c_lgtblsize);

	// Then,  keep the index modulo table size.
	index = index & c_tblmask;
	
	//
	// now increment the accumulated phase.
	//
	nco_state->current_phase = nco_state->current_phase + nco_state->phase_step; 

	// lookup the sine table.
	uint8_t retval =  ((index == 0) ? 0 : signOfSine(index));

	return(retval);
}


