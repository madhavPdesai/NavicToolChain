#ifndef _tracking_h___
#define _tracking_h___
#include <stdint.h>

//
// quantize discriminator output by using 4 values.
//
#define __QUANTIZE__(x) ((x < 0.25) ? 1 : ((x < 0.5) ? 2 : ((x < 0.75)  ? 3 : 4)))

typedef struct TrackingLoopState__ {
	

	//
	// loop counter that increments every ms.
	//
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

	// freq deviation filter.
	float k0_f;
	float k1_f;

	// phase (theta) filter.
	float k0_p;
	float k1_p;

	// code discriminator error
	// filter.
	float k0_d;
	float k1_d;

	// coherent integration interval in ms.
	int coherent_integration_interval_in_ms;
	int correlator_code_phase_separation;

	// these two constants depend on the number of milli-seconds
	// used for coherent integration.
	float residual_phase_scale_factor;
	float theta_scale_factor;
	
	// lock parameters.
	float  lock_threshold;
	float  snr_lock_threshold;
	uint8_t lock_computation_interval;
	uint8_t max_out_of_lock_iterations;
	

} TrackingLoopState;

void    configureTrackingLoop (
				// coherent integration interval in ms.
				int number_of_ms_to_integrate,
				int correlator_code_phase_separation,
				//
				// IIR filters used in the tracking loop.
				//
				float filt_theta_k0, float filt_theta_k1,
				float filt_freq_k0,  float filt_freq_k1,
				float filt_discr_err_k0, float filt_discr_err_k1,
				//
				// lock and cnr computation
				//
				float  lock_threshold,     // a number between 0.0 and 1.0, typically 0.7
				float  snr_lock_threshold, // typically 8.0 corresponding to  9dB
				uint8_t lock_computation_interval, // typically 20ms,1 bit period.
				uint8_t max_out_of_lock_iterations, // typically  8 lock computes (160ms)
				// tracking loop to be configured.
				TrackingLoopState* tls
			);

void    updateLockAndCnrInformation(float ip, float qp, TrackingLoopState* tls);
int32_t addTrackingCodePhases( int32_t cphase, int32_t increment);
void 	initTrackingLoopState (
				int32_t initial_code_delay,
				float nominal_if,
				float  initial_carrier_frequency,
				float  initial_carrier_phase,
				TrackingLoopState* tls
			   );
int runTrackingLoopIteration(
		float ie, 
		float qe,
		float ip,
		float qp,
		float il,
		float ql,
		TrackingLoopState* tls
	); 

				

#endif
