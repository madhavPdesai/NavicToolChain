#ifndef hook_h____
#define hook_h____

#define QSIZE 		64
#define QSIZE_MASK	63

typedef struct __SatelliteHook {

	int sat_id;
	int band;

	// circulating increment as you write.
	int write_pointer;

	float ip[QSIZE];
	float qp[QSIZE];
	float doppler[QSIZE];
	uint32_t code_phase[QSIZE];
	uint32_t ms_count[QSIZE];
	float snr_estimate[QSIZE];
	float lock_val[QSIZE];

	

	// set if lock is lost.
	uint8_t  lost_lock;
} SatelliteHook;

void initSatelliteHook(SatelliteHook* sh, int sat_id, int sat_band);
void printSatelliteHook(SatelliteHook* sh);
void initApplicationBody();
void printSatelliteHookStats();

// return 1 if some satellite has lost lock.
int applicationLoopBody();
int satelliteScan();

void absorbQp( SatelliteHook* sh,
		uint32_t ms_count, 
		float ip, 
		float qp, 
		uint32_t code_phase,
		float doppler,
		float snr_est,
		float lock_val);



#endif
