////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//This function takes six values of (both I & Q of early, prompt, and late) tracking correlator outputs and 
//provide NCO updates (Carrier freq & phase, three code delays) for the next iteration for the specified 
//tracking channel (given by the combination of band_id and sateliite_id)
//
//Return value: '1' indicates Tracking to be continued; '0' indicates tracking to be discontinued (track lost)
//
//Added one more input parameter 'reset_flag'
//
//When calling this function for the first time after acquiring/re-acquiring the signal, the reset_flag is set   
//to '1' and initial values of carrier frequency & code phase values obtained from Acquisition module to be   
//passed for initialising that tracking channel. On other occasions, it is set to '0'. 
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <math.h>
#include <navic_includes.h>
#include <tracking.h>
#include <app_defines.h>
#include <utils.h>

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
	float E, L;

	// 1ms time advance from last call.
	float residual_phase = (tls->carrier_frequency*2.0*M_PI*0.001);
	tls->accumulated_residual_phase += normalizeTo2Pi(residual_phase);

	float r = (ip/(tls->d_offset + qp));
	tls->theta  = atan(r);

	// phase filter.
	tls->filt_theta = tls->k1_p * tls->theta  + tls->k2_p * tls->filt_theta;
	tls->accumulated_phase += tls->filt_theta;
	tls->carrier_phase = normalizeTo2Pi(tls->accumulated_residual_phase + tls->accumulated_phase);

	// frequency filter.
	tls->freq_dev       = tls->theta*1000.0/(2*M_PI);
	tls->filt_freq_dev  = tls->k1_f*tls->freq_dev + tls->k2_f*tls->filt_freq_dev;

	tls->carrier_frequency += tls->filt_freq_dev;


	///------------------- Code Tracking Loop ----------------------------
	// Calculate the Envelope values
	E = sqrt((ie*ie)+(qe*qe));
	//P = sqrt(pow(ip,2)+pow(qp,2)); // Prompt Envelope
	L = sqrt((il*il)+(ql*ql)); // Late   Envelope

	////  E-L (Code Discriminator):
	float code_error_prev = tls->code_error;
	tls->code_error = (E - L)/(E + L);
	// Check E-L and update code phase accordingly

	// check if E-L curve has a zero crossing or not
	if ((tls->code_error * code_error_prev) >= 0) // No zero crossing
	{
		if (tls->code_error > 0) 	 // Positive, shift to left
			tls->prompt_code_delay = addTrackingCodePhases(tls->prompt_code_delay,-1);
		else if (tls->code_error < 0) // Negative, shift to right
			tls->prompt_code_delay = addTrackingCodePhases(tls->prompt_code_delay,1);
	}

	// generate early and late code delays from the prompt code delay.
	tls->early_code_delay  = addTrackingCodePhases(tls->prompt_code_delay, 
			-  TRACKING_SAMPLES_PER_HALF_CODE_CHIP);
	tls->late_code_delay   = addTrackingCodePhases(tls->prompt_code_delay,  
			TRACKING_SAMPLES_PER_HALF_CODE_CHIP);

	updateLockAndCnrInformation(ip,qp,tls);


	tls->loop_count += 1;
	return(tls->lock_status);
}

void updateLockAndCnrInformation(float ip, float qp, TrackingLoopState* tls)
{
	int lock_boundary = (tls->lock_counter == LOCK_COMPUTATION_INTERVAL);
	if(lock_boundary)
	// update the lock information on the lock boundary
	{
		if(tls->lock_status)
		// makes sense continue only if you are still locked.
		{	
			tls->lock_value = 
				(tls->qp2_sum - tls->ip2_sum)/(tls->qp2_sum + tls->ip2_sum);

			if(tls->lock_value > LOCK_THRESHOLD)
			{
				tls->snr_estimate = ((tls->qp2_sum - tls->ip2_sum)/tls->ip2_sum);
			}
			else 
			{
				// lock lost, snr values meaningless.
				tls->snr_estimate = 1.0;
			}

			int8_t curr_lock_status = 
				((tls->lock_value > LOCK_THRESHOLD) &&
				 (tls->snr_estimate > SNR_LOCK_THRESHOLD));

			if(!curr_lock_status)
			// first provisionally lose lock.
			{
				tls->out_of_lock_count++;
				if(tls->provisional_lock_status)
				{
					tls->provisional_lock_status = 0;
				}
				else if(tls->out_of_lock_count >=  MAX_OUT_OF_LOCK_ITERATIONS)
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


