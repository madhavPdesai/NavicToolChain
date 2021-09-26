#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <cp_rf_parameters.h>
#include <all_sats_prn_gen.h>
#include <all_sats_prn_keys.h>


// G1: X10 + X3 + 1 
//    initial state 1,1,1,1,1,1,1,1,1,1

// G2: X10 +X9 + X8 + X6 +X3 + X2 + 1
//    initial states as shown below.
int G2_INIT_VALUE[64*10];

			
void fillG2InitValue(int sat_id, uint32_t init_val, int* ptr)
{
	int I;

#ifdef ___DEBUGPRINT___
	fprintf(stdout,"\n// Info: sat %d shift-register init value = ", sat_id);
#endif
	for(I = 0; I < 10; I++)
	{
		uint32_t mask = (1 << (9 - I));

#ifdef ___DEBUGPRINT___
		if(I > 0)
			fprintf(stdout,", ");
#endif

		if((mask & init_val) != 0)
			ptr[I] = 1;
		else
			ptr[I] = 0;

#ifdef ___DEBUGPRINT___
		fprintf(stdout," %d", ptr[I]);
#endif
	}
#ifdef ___DEBUGPRINT___
	fprintf(stdout,"\n");
#endif
}

// return 0 on success.
int fillPrnG2InitValues (FILE* fp)
{
	int err = 0;
	char line_buffer[1024];
	while(1)
	{
		char* rb = fgets (line_buffer, 1023, fp);
		if(rb == NULL)
			break;

		if(feof(fp))
			break;

		// skip comment line
		if(line_buffer[0] == '!')
			continue;

		int sat_id;
		uint32_t init_val;
		int vals =	sscanf(line_buffer,"%d 0x%x", &sat_id, &init_val);
		if(vals < 2)
			continue;
	

		if(sat_id > 0)
		{
			fprintf(stderr,"Info: filling init poly for sat %d using 0x%x\n",
					sat_id, init_val);
		
			int* ptr = &(G2_INIT_VALUE[10*(sat_id - 1)]);

			fillG2InitValue(sat_id, init_val, ptr);
		}
		else
		{
			fprintf(stderr,"Error: sat_id %d must be > 0\n", sat_id);
			err = 1;
		}
	}

	return(err);
}


//
// sat id must be at most 14.
// init-vector must be an int vector with 10 entries.
//
void initPrnGen (PrnGenState* prn_gen_state, int* init_vector)
{
	int I;
	for(I=1; I <= 10; I++)
	{
		prn_gen_state->G1[I] = 1;

		int J = (10 - I);
		prn_gen_state->G2[I] = init_vector[J];
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

//
// new function added.
//  generate PRN sequence for satellite_id, and store it
//  in the prn_buffer.  The PRN buffer must have at least
//  1024 bytes.
//
// return 0 on success.
//
int generatePrnSequence(uint8_t satellite_id, int8_t* prn_buffer)
{
	if((satellite_id == 0) || (satellite_id > 64))
		return(1);

	int I;

	// initial value in G2.
	int init_vector[10];
	fillG2InitValue(satellite_id, all_sats_prn_keys[satellite_id - 1], init_vector);

	// initialize the PRN generator.
	PrnGenState prn_gen_state;
	initPrnGen(&prn_gen_state, init_vector);

	// generate 1023 prn bits.
	for(I = 0; I < 1023; I++)
	{
		int raw_val = prnGenTick(&prn_gen_state);	
		prn_buffer[I] =  ((raw_val == 0) ? -1 : 1);
	}
	
	// wrap around.
	prn_buffer[1023] = prn_buffer[0];

	return(0);
}

