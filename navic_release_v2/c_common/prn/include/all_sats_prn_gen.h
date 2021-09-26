#ifndef ALL_SATS_PRN_GEN_H__
#define ALL_SATS_PRN_GEN_H__
#include <stdint.h>

typedef struct PrnGenState__ {

	// G1[1:10] is first shift register.
	int G1[11];

	// G2[1:10] is the second shift register.
	int G2[11];	

} PrnGenState;


void fillG2InitValue(int sat_id, uint32_t init_val, int* init_vector);
int fillPrnG2InitValues (FILE* fp);
void initPrnGen (PrnGenState* prn_gen_state, int* init_vector);
int  prnGenTick  (PrnGenState* prn_gen_state);
//
// new function added.
//  generate PRN sequence for satellite_id, and store it
//  in the prn_buffer.  The PRN buffer must have at least
//  1024 bytes.
//
// return 0 on success.
//
int generatePrnSequence(uint8_t satellite_id, int8_t* prn_buffer);
#endif
