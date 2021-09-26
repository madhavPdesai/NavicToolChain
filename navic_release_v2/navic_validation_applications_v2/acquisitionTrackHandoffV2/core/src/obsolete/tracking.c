#include <stdint.h>
#include <math.h>
#include <app_defines.h>
#include <tracking.h>

void setTrackingLoopParameters (TrackingLoopState* tls, 
				 int lock_computation_interval,
				 float d_offset, float k1_f, float k2_f,
				 float k1_p, float k2_p)
{
	tls->d_offset = d_offset;
	tls->k1_f    = k1_f;
	tls->k2_f    = k2_f;
	tls->k1_p    = k1_p;
	tls->k2_p    = k2_p;
}


void initTrackingLoopState (
				int32_t initial_code_delay,
				float  nominal_if,
				float  initial_carrier_frequency,
				float  initial_carrier_phase,
				TrackingLoopState* tls)
{
	tls->loop_count = 1;


	tls->provisional_lock_status = 1;
	tls->lock_status = 1;

	tls->out_of_lock_count = 0;
	tls->lock_counter = 0;

	tls->ip2_sum = 0;
	tls->qp2_sum = 0;
	tls->snr_estimate = 1.0;
	tls->lock_value = 0.0;

	tls->nominal_if = nominal_if;
	tls->carrier_frequency = initial_carrier_frequency;
	tls->carrier_phase     = initial_carrier_phase;
	tls->prompt_code_delay = initial_code_delay;
	tls->early_code_delay  = addTrackingCodePhases(initial_code_delay, 
							- TRACKING_SAMPLES_PER_HALF_CODE_CHIP);
	tls->late_code_delay   = addTrackingCodePhases(initial_code_delay, 
							 TRACKING_SAMPLES_PER_HALF_CODE_CHIP);

	tls->accumulated_residual_phase = 0;
	tls->accumulated_phase = 0;
	tls->theta = initial_carrier_phase;

	tls->filt_theta = 0.0;
	tls->freq_dev = 0.0;
	tls->filt_freq_dev = 0;
	tls->code_error = 0;
}


