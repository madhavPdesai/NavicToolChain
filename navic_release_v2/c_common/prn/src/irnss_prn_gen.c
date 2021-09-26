#include <stdio.h>
#include <stdlib.h>
#include <irnss_prn_gen.h>


// G1: X10 + X3 + 1 
//    initial state 1,1,1,1,1,1,1,1,1,1

// G2: X10 +X9 + X8 + X6 +X3 + X2 + 1
//    initial states as shown below.
int G2_POLYNOMIAL_INIT_STATE_L5_BAND[] =
	{
		0,0,0,0,0,0,0,0,0,0, // not used.
 		1,1,1,0,1,0,0,1,1,1,
 		0,0,0,0,1,0,0,1,1,0,
 		1,0,0,0,1,1,0,1,0,0,
 		0,1,0,1,1,1,0,0,1,0,
 		1,1,1,0,1,1,0,0,0,0,
		0,0,0,1,1,0,1,0,1,1,
 		0,0,0,0,0,1,0,1,0,0,
 		0,1,0,0,1,1,0,0,0,0,
 		0,0,1,0,0,1,1,0,0,0,
		1,1,0,1,1,0,0,1,0,0,
		0,0,0,1,0,0,1,1,0,0,
		1,1,0,1,1,1,1,1,0,0,
		1,0,1,1,0,1,0,0,1,0,
		0,1,1,1,1,0,1,0,1,0
	};

int G2_POLYNOMIAL_INIT_STATE_S_BAND[] =
	{
		0,0,0,0,0,0,0,0,0,0, // not used.
 		0,0,1,1,1,0,1,1,1,1,
 		0,1,0,1,1,1,1,1,0,1,
 		1,0,0,0,1,1,0,0,0,1,
 		0,0,1,0,1,0,1,0,1,1,
 		1,0,1,0,0,1,0,0,0,1,
 		0,1,0,0,1,0,1,1,0,0,
 		0,0,1,0,0,0,1,1,1,0,
 		0,1,0,0,1,0,0,1,1,0,
 		1,1,0,0,0,0,1,1,1,0,
 		1,0,1,0,1,1,1,1,1,0,
 		1,1,1,0,0,1,0,0,0,1,
 		1,1,0,1,1,0,1,0,0,1,
 		0,1,0,1,0,0,0,1,0,1,
 		0,1,0,0,0,0,1,1,0,1
	};


// sat id must be at most 14.
void initPrnGen (int band, int sat_id, PrnGenState* prn_gen_state)
{
	prn_gen_state->band = band;
	prn_gen_state->sat_id = sat_id;

	int I;
	for(I=1; I <= 10; I++)
	{
		prn_gen_state->G1[I] = 1;
		int J = (11 - I);
		prn_gen_state->G2[I] = 
				((band == 1)  ?
					G2_POLYNOMIAL_INIT_STATE_L5_BAND[(sat_id*10) + (J-1) ] :
					G2_POLYNOMIAL_INIT_STATE_S_BAND[(sat_id*10) + (J-1) ]);
		
	}
}

int  prnGenTick  (PrnGenState* prn_gen_state)
{
	int ret_val = prn_gen_state->G1[10] ^ prn_gen_state->G2[10];

	// 3,10
	int nG1_1 = prn_gen_state->G1[3] ^ prn_gen_state->G1[10];

	//2,3,6,8,9,10
	int nG2_1 = prn_gen_state->G2[2] ^ prn_gen_state->G2[3] ^ prn_gen_state->G2[6] ^ 
			prn_gen_state->G2[8] ^ prn_gen_state->G2[9] ^ prn_gen_state->G2[10];

	// shift.
	int I;
	for(I=10; I >= 2; I--)
	{
		prn_gen_state->G1[I] = prn_gen_state->G1[I-1];
		prn_gen_state->G2[I] = prn_gen_state->G2[I-1];
	}

	prn_gen_state->G1[1] = nG1_1;
	prn_gen_state->G2[1] = nG2_1;

	return(ret_val);
}

