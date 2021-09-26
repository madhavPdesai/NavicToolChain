#ifndef _tracking_h___
#define _tracking_h___

#include "app_defines.h"

typedef struct TrackingLoopState__ {

	int loop_count;

	//  lock related stuff.
	//  keep count of ms intervals for determining lock.
	int8_t lock_counter;
	//  provisional lock status.
	int8_t provisional_lock_status;
	//  for how many intervals have you been 
	//  out of lock?
	int8_t out_of_lock_count;
	//  the lock status.  Once this becomes 0
	//  you are out of lock and luck.
	int8_t lock_status;

	// fine track.
	int8_t fine_track_counter;

	// accumulated values for coherent integration.
	float ip_a;
	float qp_a;
	float ie_a;
	float qe_a;
	float il_a;
	float ql_a;

	// accumulated values for SNR estimation.
	float ip2_sum;
	float qp2_sum;

	float snr_estimate;
	float lock_value;

	//
	// Current values
	//
	float nominal_if;
	// nominal_if + doppler.
	float carrier_frequency;
	float carrier_phase;

	int32_t early_code_delay;
	int32_t prompt_code_delay;
	int32_t late_code_delay;


	float accumulated_residual_phase;
	float accumulated_phase;

	float theta;
	float filt_theta;
	float freq_dev;
	float filt_freq_dev;

	//
	// internal state  for DLL
	//
	// accumulated values.
	//
	int   dll_counter;

	float code_error;
	float filt_code_error;
	float projected_prompt_code_delay;

	// loop parameters
	float d_offset;
	float k1_f;
	float k2_f;

	float k1_p;
	float k2_p;

	float k1_c;
	float k2_c;

} TrackingLoopState;

void updateLockAndCnrInformation(float ip, float qp, TrackingLoopState* tls);

int32_t addTrackingCodePhases(int32_t cphase, int32_t increment);
void 	setTrackingLoopParameters (TrackingLoopState* tls, 
					int lock_computation_interval,
				 	float d_offset, float k1_f, float k2_f,
					float k1_p, float k2_p, float k1_c, float k2_c);
void initTrackingLoopState (
				int32_t initial_code_delay,
				float nominal_if,
				float  initial_carrier_frequency,
				float  initial_carrier_phase,
				TrackingLoopState* tls);
int runTrackingLoopIteration(
		float ie, 
		float qe,
		float ip,
		float qp,
		float il,
		float ql,
		TrackingLoopState* tls
	); 
int runTrackingLoopIterationAlternate(
		float ie, 
		float qe,
		float ip,
		float qp,
		float il,
		float ql,
		TrackingLoopState* tls
	); 

				

#endif
