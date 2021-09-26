#ifndef nco8bit_h____
#define nco8bit_h____
#include <cp_rf_parameters.h>

/*****************************************************************************************
 NCO block
   (this is maintained in the cp engine hardware).
******************************************************************************************/
#define	ONE_NCO_ROTATION   (2.0*(1u<<31))
#define	ONE_NCO_ROTATION_64BIT   (2.0*(((uint64_t) 1) << 63))
#define	ONE_NCO_ROTATION_61BIT   (2.0*(((uint64_t) 1) << 60))

typedef struct __Nco8bitState {
	int8_t cos_table[RF_COS_TABLE_SIZE/2];
	uint32_t current_phase;
	uint32_t phase_step;
} Nco8bitState;

void    initNco8bitState  (Nco8bitState* nco_state, 
				double sampling_frequency, 
				double signal_frequency,
				double initial_phase);
int8_t  nextNco8bitOutput (Nco8bitState* nco_state);

#endif
