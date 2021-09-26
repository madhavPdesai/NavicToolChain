#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cp_rf_parameters.h>
#include <nco.h>

//
// NCO generator code.
//
int 	  c_3lgtblsize = 4;
uint32_t  c_3tblsize   = 16;

int8_t sine_table[] = {0,1,2,2,3,2,2,1};
int8_t cos_table[]  = {3,2,2,1,0,-1,-2,-2};



// 2-bit sign magnitude..
int8_t sine3Value (uint32_t idx)
{
	idx = idx & 0xf;

	int8_t ret_val;

	if(idx >= 8)
		ret_val = - sine_table[idx - 8];
	else
		ret_val = sine_table[idx];

	return((int8_t) ret_val);
}

int8_t cos3Value (uint32_t idx)
{
	idx = idx & 0xf;

	int8_t ret_val;

	if(idx >= 8)
		ret_val = - cos_table[idx - 8];
	else
		ret_val = cos_table[idx];

	return((int8_t) ret_val);
}


void nextNco3Output (NcoState* nco_state, int8_t* sine_val, int8_t* cos_val)
{
	// keep top 4 bits
	uint32_t index = (nco_state->current_phase >> (32 - c_3lgtblsize));
	//fprintf(stderr,"nco3: nco_state->current_phase = 0x%x\n", nco_state->current_phase);

	nco_state->current_phase = nco_state->current_phase + nco_state->phase_step; 
	*sine_val =  sine3Value(index);
	*cos_val  =  cos3Value(index);

	return;
}


