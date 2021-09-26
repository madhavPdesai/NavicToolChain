#include <stdint.h>
#include <math.h>
#include "iitbnavic_globals.h"
#include "tracking_loop.h"

// normalize to pi.
float normalizeTo2Pi(float x)
{               
        float y = ((x < 0) ? -x : x);
        int R = (y * (0.5/M_PI));

        y = y - (2.0*M_PI*R);

        y = ((x < 0) ? ((2.0*M_PI) - y) : y);

        return(y);
}       


int32_t addTrackingCodePhases( int32_t cphase, int32_t increment)
{
	int32_t result = cphase + increment;
	if(result < 0)
	{
		result = result + IITBNAVIC_TRACKING_SAMPLES_PER_CODE;
	}
	else if(result > IITBNAVIC_TRACKING_SAMPLES_PER_CODE)
	{
		result = result - IITBNAVIC_TRACKING_SAMPLES_PER_CODE;
	}
	return(result);
}


void configureTrackingLoop (
				// coherent integration interval in ms.
				int number_of_ms_to_integrate,
				int correlator_code_phase_separation,
				//
				// IIR filters used in the tracking loop.
				//
				float filt_theta_k0, float filt_theta_k1,
				float filt_freq_k0,  float filt_freq_k1,
				float filt_discr_err_k0, float filt_discr_err_k1,
				// lock params.
				float  lock_threshold,
				float  snr_lock_threshold,
				uint8_t lock_computation_interval,
				uint8_t max_out_of_lock_iterations,
				// Tracking loop state.
				TrackingLoopState* tls)
{
	if (number_of_ms_to_integrate > 0)
	{
		tls->coherent_integration_interval_in_ms = number_of_ms_to_integrate;
		tls->residual_phase_scale_factor = (2.0*M_PI*0.001*number_of_ms_to_integrate);
		tls->theta_scale_factor =  (1000.0/(2*M_PI*number_of_ms_to_integrate));
	}
	tls->correlator_code_phase_separation = correlator_code_phase_separation;


	tls->k0_f = filt_freq_k0;
	tls->k1_f = filt_freq_k1;

	tls->k0_p = filt_theta_k0;
	tls->k1_p = filt_theta_k1;

	tls->k0_d = filt_discr_err_k0;
	tls->k1_d = filt_discr_err_k1;

	// reset the fine track counter.
	tls->fine_track_counter = 0;

	// lock related.
	tls->lock_threshold = lock_threshold;
	tls->snr_lock_threshold = snr_lock_threshold;
	tls->lock_computation_interval = lock_computation_interval;
	tls->max_out_of_lock_iterations = max_out_of_lock_iterations;
}

int runTrackingLoopIteration(
		float ie, 
		float qe,
		float ip,
		float qp,
		float il,
		float ql,
		TrackingLoopState* tls
		) 
{

	tls->ip_a += ip;
	tls->qp_a += qp;
	tls->ie_a += ie;
	tls->qe_a += qe;
	tls->il_a += il;
	tls->ql_a += ql;
	tls->fine_track_counter++;

	if(tls->fine_track_counter == tls->coherent_integration_interval_in_ms)
	{
		float r = (tls->ip_a/(1.0 + tls->qp_a));
		tls->theta  = atan(r);

		// 1ms X coherent-integration  time advance from last call.
		float residual_phase = (tls->carrier_frequency*tls->residual_phase_scale_factor);
		tls->accumulated_residual_phase += normalizeTo2Pi(residual_phase);

		// phase filter.
		tls->filt_theta = tls->k0_p * tls->theta  + tls->k1_p * tls->filt_theta;
		tls->accumulated_phase += tls->filt_theta;
		tls->carrier_phase = normalizeTo2Pi(tls->accumulated_residual_phase + tls->accumulated_phase);

		// frequency filter.
		tls->freq_dev       = tls->theta*tls->theta_scale_factor;

		tls->filt_freq_dev  = tls->k0_f*tls->freq_dev + tls->k1_f*tls->filt_freq_dev;

		tls->carrier_frequency += tls->filt_freq_dev;


		///------------------- Code Tracking Loop ----------------------------
		// Calculate the Envelope values
		float E = sqrt((tls->ie_a*tls->ie_a)+(tls->qe_a*tls->qe_a));
		float L = sqrt((tls->il_a*tls->il_a)+(tls->ql_a*tls->ql_a));

		float EmL = E - L;
		tls->code_error = EmL / (E + L);

		//  E-L code error filter... simple IIR filter.
		tls->filt_code_error = 
			(tls->k0_d * tls->code_error) + (tls->k1_d * tls->filt_code_error);


		// multi-level quantizer.. 
		float absfe = fabs(tls->filt_code_error);
		int code_step = __QUANTIZE__(absfe);


		// the projected value is more precise than the quantized value.
		float code_delay_adjustment = (tls->correlator_code_phase_separation * tls->filt_code_error);


		// Comparator..
		if (tls->filt_code_error > 0.0) 	 // Positive, shift to left
		{
			tls->prompt_code_delay = addTrackingCodePhases( tls->prompt_code_delay,-code_step);
			tls->projected_prompt_code_delay = tls->prompt_code_delay - code_delay_adjustment;
		}
		else if (tls->filt_code_error <  0.0) // Negative, shift to right
		{
			tls->prompt_code_delay = addTrackingCodePhases(tls->prompt_code_delay,code_step);
			tls->projected_prompt_code_delay = tls->prompt_code_delay + code_delay_adjustment;
		}

		// generate early and late code delays from the prompt code delay.
		tls->early_code_delay  = addTrackingCodePhases(tls->prompt_code_delay, 
								-  tls->correlator_code_phase_separation);
		tls->late_code_delay   = addTrackingCodePhases(tls->prompt_code_delay,  
								tls->correlator_code_phase_separation);


		tls->fine_track_counter = 0;
		tls->ip_a = 0;
		tls->qp_a = 0;
		tls->ie_a = 0;
		tls->qe_a = 0;
		tls->il_a = 0;
		tls->ql_a = 0;
	}

	updateLockAndCnrInformation(ip,qp,tls);

	tls->loop_count += 1;
	return(tls->lock_status);
}

void updateLockAndCnrInformation(float ip, float qp, TrackingLoopState* tls)
{
	int lock_boundary = (tls->lock_counter == tls->lock_computation_interval);
	if(lock_boundary)
		// update the lock information on the lock boundary
	{
		if(tls->lock_status)
			// makes sense continue only if you are still locked.
		{	
			tls->lock_value = 
				(tls->qp2_sum - tls->ip2_sum)/(tls->qp2_sum + tls->ip2_sum);

			if(tls->lock_value > tls->lock_threshold)
			{
				tls->snr_estimate = ((tls->qp2_sum - tls->ip2_sum)/tls->ip2_sum);
			}
			else 
			{
				// lock lost, snr values meaningless.
				tls->snr_estimate = 1.0;
			}

			int8_t curr_lock_status = 
				((tls->lock_value > tls->lock_threshold) &&
				 (tls->snr_estimate > tls->snr_lock_threshold));

			if(!curr_lock_status)
				// first provisionally lose lock.
			{
				tls->out_of_lock_count++;
				if(tls->provisional_lock_status)
				{
					tls->provisional_lock_status = 0;
				}
				else if(tls->out_of_lock_count >=  tls->max_out_of_lock_iterations)
					// check if you have been out of lock long enough.
				{
					tls->lock_status = 0;
				}
			}
			else
				// lock value is high.  back into lock.
			{
				tls->provisional_lock_status = 1;
				tls->lock_status = 1;
				tls->out_of_lock_count = 0;

			}

			tls->lock_counter = 0;
			tls->ip2_sum = 0;
			tls->qp2_sum = 0;
		}
	}

	tls->ip2_sum += ip*ip;
	tls->qp2_sum += qp*qp;

	tls->lock_counter++;
}


