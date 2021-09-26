#ifndef _satellite_h___
#define _satellite_h___

#include <stdint.h>
#include <pthread.h>
#include <tracking.h>

// 32-bit mask for satellite.
// bit   interpretation
// 0     activated
// 1     coarse-acquire-started
// 2     coarse-acquire-done
// 3     fine-acquire-started
// 4     fine-acquire-done
// 5     positive-doppler-track-started
// 6     positive-doppler-track-done
// 7     negative-doppler-track-started
// 8     negative-doppler-track-done
// 9     tracking-started
// 10    track-lost
//
// rest unused.
//
#define  ACTIVATED_MASK  			0x1    
#define  COARSE_ACQUIRE_STARTED_MASK  		0x2    
#define  COARSE_ACQUIRE_DONE_MASK  		0x4    
#define  FINE_ACQUIRE_STARTED_MASK  		0x8   
#define  FINE_ACQUIRE_DONE_MASK  		0x10   
#define  POSITIVE_DOPPLER_TRACK_STARTED_MASK	0x20
#define  POSITIVE_DOPPLER_TRACK_DONE_MASK	0x40
#define  NEGATIVE_DOPPLER_TRACK_STARTED_MASK   	0x80
#define  NEGATIVE_DOPPLER_TRACK_DONE_MASK    	0x100
#define  TRACK_STARTED_MASK			0x200
#define  TRACK_LOST_MASK			0x400

typedef volatile struct SatelliteTrackHistoryQueue__ {

	float ip[IQ_SAMPLE_DEPTH];
	float qp[IQ_SAMPLE_DEPTH];

	float estimated_doppler[IQ_SAMPLE_DEPTH];
	float estimated_phase[IQ_SAMPLE_DEPTH];

	int32_t prompt_code_phase[IQ_SAMPLE_DEPTH];
	uint32_t ms_count[IQ_SAMPLE_DEPTH];

	uint32_t write_pointer;
	uint32_t read_pointer;

	// mutex
	uint32_t iq_mutex;

	float snr_estimate;
	float lock_value;

} SatelliteTrackHistoryQueue;
void acquireMutexForTrackHistoryQueue(SatelliteTrackHistoryQueue* sthq);
void releaseMutexForTrackHistoryQueue(SatelliteTrackHistoryQueue* sthq);

void initTrackHistoryQueue (SatelliteTrackHistoryQueue* sthq);
int  pushIntoTrackHistoryQueue(SatelliteTrackHistoryQueue* sthq,
					uint32_t ms_count,
					float estimated_doppler, 
					float estimated_phase,
					int32_t prompt_code_phase,
					float ip, float iq,
					float snr_estimate,
					float lock_value);
// return 1 if pop succeeded.
int  popFromTrackHistoryQueue(SatelliteTrackHistoryQueue* sthq,
					uint32_t *ms_count,
					float* estimated_doppler, float* estimated_phase,
					int32_t *prompt_code_phase,
					float *ip, float *iq, float* snr_est, float* lock_val);
// return 1 if entry is valid
int getEntryInTrackHistoryQueue(SatelliteTrackHistoryQueue* sthq,
					int32_t offset_from_read_pointer,
					uint32_t *ms_count,
					float* estimated_doppler, float* estimated_phase,
					int32_t *prompt_code_phase,
					float *ip, float *iq, float* snr_est, float* lock_val);

int numberOfEntriesInTrackHistoryQueue(SatelliteTrackHistoryQueue* sthq);

// lock check on track iq results.
int lockCheckOnTrackHistoryQueue(SatelliteTrackHistoryQueue* sthq);

typedef struct SatelliteState__ {

	int8_t sat_id;
	int8_t rf_band;
	int8_t enabled;

	// prn code pointer.
	int8_t *prn_pointer;

	//
	// set if this one is a dummy.  will only
	// do fine acquire and then quit.
	//
	int dummy_satellite;

	///////////////////////////////////////////////
	// settings
	///////////////////////////////////////////////

	// coarse search parameters.
	//  code-phase: units are in 1/4 chip.
	int32_t coarse_acquisition_code_phase_min;
	int32_t coarse_acquisition_code_phase_max;
	int32_t coarse_acquisition_code_phase_step;

	// doppler search parameters for coarse search
	float coarse_acquisition_doppler_min;
	float coarse_acquisition_doppler_max;
	float coarse_acquisition_doppler_bin_size;
	float coarse_acquisition_bin_count;

	// fine search parameters
	// units are in 1/4 chip
	int32_t fine_acquisition_code_phase_min;
	int32_t fine_acquisition_code_phase_max;
	int32_t fine_acquisition_code_phase_step;

	// doppler search parameters for fine search
	float fine_acquisition_doppler_min;
	float fine_acquisition_doppler_max;
	float fine_acquisition_doppler_bin_size;

	/////////////////////////////////////////////
	// Observations
	/////////////////////////////////////////////

	// coarse doppler (sign not known)
	float  coarse_acquisition_doppler;
	int32_t coarse_acquisition_code_phase;
	uint32_t coarse_acquisition_energy;
	float  coarse_acquisition_fom;

	// fine doppler (sign not known)
	int32_t fine_acquisition_code_phase;
	float  fine_acquisition_doppler;
	uint32_t fine_acquisition_energy;

	// use the one which is higher, to
	// determine the sign of the doppler
	float  positive_doppler_track_energy;
	float  negative_doppler_track_energy;

	// 
	// state: each bit of the progress_word
	//         has a meaning.
	//
	// bit   interpretation
	// 0     activated
	// 1     coarse-acquire-started
	// 2     coarse-acquire-done
	// 3     fine-acquire-started
	// 4     fine-acquire-done
	// 5     positive-doppler-track-started
	// 6     positive-doppler-track-done
	// 7     negative-doppler-track-started
	// 8     negative-doppler-track-done
	// 9     tracking-started
	// 10    track-lost
	//
	uint32_t satellite_progress_word;
	

	TrackingLoopState tls;
	SatelliteTrackHistoryQueue sthsq;
} SatelliteState;

void initSatelliteState(SatelliteState* ss, int sat_id, int rf_band, int dummy_flag, int enable_flag);
void schedulePositiveDopplerTrack(CoprocessorState* cp_state, SatelliteState* ss);
void scheduleNegativeDopplerTrack(CoprocessorState* cp_state, SatelliteState* ss);
void scheduleNormalTracking(CoprocessorState* cp_state, SatelliteState* ss);

void setCoarseAcquireStarted(SatelliteState* ss, int val);
int  getCoarseAcquireStarted(SatelliteState* ss);

void setCoarseAcquireDone(SatelliteState* ss, int val);
int  getCoarseAcquireDone(SatelliteState* ss);

void setFineAcquireStarted(SatelliteState* ss, int val);
int  getFineAcquireStarted(SatelliteState* ss);

void setFineAcquireDone(SatelliteState* ss, int val);
int  getFineAcquireDone(SatelliteState* ss);

void setPositiveDopplerTrackStarted(SatelliteState* ss, int val);
int  getPositiveDopplerTrackStarted(SatelliteState* ss);

void setPositiveDopplerTrackDone(SatelliteState* ss, int val);
int  getPositiveDopplerTrackDone(SatelliteState* ss);

void setNegativeDopplerTrackStarted(SatelliteState* ss, int val);
int  getNegativeDopplerTrackStarted(SatelliteState* ss);

void setNegativeDopplerTrackDone(SatelliteState* ss, int val);
int  getNegativeDopplerTrackDone(SatelliteState* ss);

void setTrackingStarted(SatelliteState* ss, int val);
int  getTrackingStarted(SatelliteState* ss);

void setTrackLost(SatelliteState* ss, int val);
int  getTrackLost(SatelliteState* ss);

//
// Satellite number 1 to 32: GPS, L1 band.
// Satellite number 33: Dummy satellite for L1 floor estimation, uses IRNSS 1 PRN
// Satellite number 34 to 47:  IRNSS satellites 1 to 14, L5 band.
// Satellite number 48: Dummy satellite for L5 floor estimation, uses GPS 1 PRN.
// Satellite number 49 to 62:  IRNSS satellites 15 to 28, S band.
// Satellite number 63: Dummy satellite for S floor estimation,  uses GPS 1 PRN.
//
//
// arguments
//   gps_l1     if set, all GPS L1 satellites are initialized.
//   irnss_l5   if set, IRNSS L5 satellites are activated.
//   irnss_s    if set, IRNSS S satellites are activated.
//
// Satellite number 1 to 32: GPS, L1 band.
// Satellite number 33 to 46:  IRNSS satellites 1 to 14, L5 band.
// Satellite number 47 to 60:  IRNSS satellites 15 to 28, S band.
// Satellite number 61: Dummy satellite for L1 floor estimation, uses IRNSS 1 PRN
// Satellite number 62: Dummy satellite for L5 floor estimation, uses GPS 1 PRN.
// Satellite number 63: Dummy satellite for S floor estimation,  uses GPS 1 PRN.
//
//  In this example, we will setup the satellites as
//  follows
//       sat 33 	band l1
//	 sat 34         band l5
//       sat 47         band s
//
void initializeAllSatelliteStructures(int gps_l1, int irnss_l5, int irnss_s);

#endif

