#ifndef hook_h____
#define hook_h____

typedef struct BitDump__ {
	uint8_t   bit_counter[64];
	uint32_t  bit_vector[64];
} BitDump;
void initBitDump(BitDump* bds);
void recordBit(BitDump* bds, uint32_t ms_index, int ch, int bit);

typedef struct __SatelliteHook {

	int sat_id;
	int band;


	uint32_t bit_start_ms_count;

	uint32_t bit_start_code_phase;
	double   bit_average_code_phase;

	double   bit_start_doppler;
	double   bit_average_doppler;

	double   bit_average_qp;

	// has_bit should be 0 if 
	// this data structure is to
	// be filled.
	uint8_t has_bit;

	// value of bit
	uint8_t bit_val;

	// number of zero crossings in bit
	// interval.  should be <= 1.
	uint8_t  bit_zero_crossings;

	// set if lock is lost.
	uint8_t  lost_lock;
} SatelliteHook;

void initSatelliteHook(SatelliteHook* sh, int sat_id, int sat_band);
void initApplicationBody();
void applicationLoopBody();
void satelliteScan();


#endif
