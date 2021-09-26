#ifndef _tracking_h___
#define _tracking_h___

#include "app_defines.h"

typedef struct TrackingLoopState__ {
	int loop_count;

	int8_t satellite_index;

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
	// +1 for positive qp, -1 for negative qp.
	int8_t qp_sign_changes;

	// track integration interval in ms.
	int8_t carrier_integration_interval_in_ms;
	int8_t code_phase_integration_interval_in_ms;

	// fine track.
	int8_t carrier_track_counter;
	int8_t code_phase_track_counter;

	// accumulated values for coherent carrier integration.
	double ip_a;
	double qp_a;
	double ie_a;
	double qe_a;
	double il_a;
	double ql_a;

	// accumulated values for coherent code phase integration.
	double cp_ie_a;
	double cp_qe_a;
	double cp_il_a;
	double cp_ql_a;

	// accumulated values for SNR estimation.
	double ip2_sum;
	double qp2_sum;
	double last_qp;

	double snr_estimate;
	double lock_value;

	//
	// Current values
	//
	//  IF + doppler..
	double carrier_frequency;
	//  radians normalized to [0,2pi)
	double carrier_phase;

	int32_t early_code_delay;
	int32_t prompt_code_delay;
	int32_t late_code_delay;


	double accumulated_residual_phase;
	double accumulated_phase;

	double theta;
	double filt_theta;
	double freq_dev;
	double filt_freq_dev;

	//
	// internal state  for DLL
	//
	// accumulated values.
	//
	int   dll_counter;

	double code_error;
	double filt_code_error;
	double projected_prompt_code_delay;

	// loop parameters
	float d_offset;
	float k1_f;
	float k2_f;

	float k1_p;
	float k2_p;

	float k1_c;
	float k2_c;
} TrackingLoopState;

void updateLockAndCnrInformation(double ip, double qp, TrackingLoopState* tls);

int32_t addTrackingCodePhases(int32_t cphase, int32_t increment);
void 	setTrackingLoopParameters (TrackingLoopState* tls, 
					int lock_computation_interval,
				 	double d_offset, 
					double k1_f, double k2_f,
					double k1_p, double k2_p,
					double k1_c, double k2_c);
void initTrackingLoopState (
				int8_t  carrier_integration_interval_in_ms,
				int8_t  code_phase_integration_interval_in_ms,
				int32_t initial_code_delay,
				double  initial_carrier_frequency,
				double  initial_carrier_phase,
				TrackingLoopState* tls
			  );

int runTrackingLoopIteration(
		double ie, 
		double qe,
		double ip,
		double qp,
		double il,
		double ql,
		TrackingLoopState* tls
	); 
int runTrackingLoopIterationAlternate(
		double ie, 
		double qe,
		double ip,
		double qp,
		double il,
		double ql,
		TrackingLoopState* tls
	); 

				

#endif
