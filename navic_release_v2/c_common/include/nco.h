#ifndef nco_h____
#define nco_h____

#define CHIPS_PER_MS				1023
#define RF_OVERSAMPLING_FACTOR 			16
#define RF_BLOCK_QUEUE_SIZE 			8
#define INTERNAL_SAMPLING_FREQUENCY             16*CHIPS_PER_MS*1000
#define INTERNAL_INTERMEDIATE_FREQUENCY		(4*CHIPS_PER_MS*1000)

// sampling frequency is 16x1.023 MHz.  The number of
// samples in the table is thus 2**14.
#define LOG_SINE_SIGN_TABLE_SIZE 		14 
#define SINE_SIGN_TABLE_SIZE 			(1 << 14)
#define SINE_SIGN_TABLE_MASK 			((1 << 14) - 1)

/*****************************************************************************************
 NCO block
   (this is maintained in the cp engine hardware).
******************************************************************************************/
#define	ONE_NCO_ROTATION   	 (2.0*(1u<<31))
#define	ONE_NCO_ROTATION_64BIT   (2.0*(((uint64_t) 1) << 63))
#define	ONE_NCO_ROTATION_61BIT   (2.0*(((uint64_t) 1) << 60))
#define tb_inv_2pi  (1.0/(2.0*M_PI))
#define c_tb_tblsize  SINE_SIGN_TABLE_SIZE; 

typedef struct __NcoState {
	uint32_t current_phase;
	uint32_t phase_step;
} NcoState;

uint8_t  signOfSine (uint32_t idx);
void     initNcoState (NcoState* nco_state, uint32_t initial_phase, uint32_t phase_step);
uint8_t  nextNcoOutput (NcoState* nco_state);

int8_t sine3Value (uint32_t idx);
int8_t cos3Value (uint32_t idx);
void nextNco3Output (NcoState* nco_state, int8_t* sine_val, int8_t* cos_val);

#endif
