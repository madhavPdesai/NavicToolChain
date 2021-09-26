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


//#define __QUANTIZE__(x) ((x < 0.5) ? 1 : 3)
//#define __QUANTIZE__(x) ((x < 0.5) ? 1 : 3)
//#define __QUANTIZE__(x) ((x > EL_EPSILON) ? 1 : 0)

//
// quantize by truncating using 4 values.
//
#define __QUANTIZE__(x) ((x < 0.25) ? 1 : ((x < 0.5) ? 2 : ((x < 0.75)  ? 3 : 4)))

#define RPHASE_SCALE_FACTOR (2.0*M_PI*0.001*NMS)
#define THETA_SCALE_FACTOR (1000.0/(2*M_PI*NMS))

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

	if(tls->fine_track_counter == TRACK_NMS)
	{
		float r = (tls->ip_a/(tls->d_offset + tls->qp_a));
		tls->theta  = atan(r);

		// 1ms X coherent-integration  time advance from last call.
		float residual_phase = (tls->carrier_frequency*RPHASE_SCALE_FACTOR);
		tls->accumulated_residual_phase += normalizeTo2Pi(residual_phase);

		// phase filter.
		tls->filt_theta = tls->k1_p * tls->theta  + tls->k2_p * tls->filt_theta;
		tls->accumulated_phase += tls->filt_theta;
		tls->carrier_phase = normalizeTo2Pi(tls->accumulated_residual_phase + tls->accumulated_phase);

		// frequency filter.
		tls->freq_dev       = tls->theta*THETA_SCALE_FACTOR;

		tls->filt_freq_dev  = tls->k1_f*tls->freq_dev + tls->k2_f*tls->filt_freq_dev;

		tls->carrier_frequency += tls->filt_freq_dev;


		///------------------- Code Tracking Loop ----------------------------
		// Calculate the Envelope values
		float E = sqrt((tls->ie_a*tls->ie_a)+(tls->qe_a*tls->qe_a));
		float L = sqrt((tls->il_a*tls->il_a)+(tls->ql_a*tls->ql_a));

#if USE_NEW_DISCRIMINATOR
		float P = sqrt((tls->ip_a*tls->ip_a)+(tls->qp_a*tls->qp_a));
#endif
		float EmL = E - L;

		//
		// discriminator: note unbiased version..
		//   From Xie and Yuan.
		//
#if USE_NEW_DISCRIMINATOR
		tls->code_error = EmL / ((2.0*P) + fabs(EmL) - (E + L));
#else
		tls->code_error = EmL / (E + L);
#endif

		//  E-L code error filter... simple IIR filter.
		tls->filt_code_error = 
			(EL_FILTER_K0 * tls->code_error) + (EL_FILTER_K1 * tls->filt_code_error);


		// multi-level quantizer.. 
		float absfe = fabs(tls->filt_code_error);
		int code_step = __QUANTIZE__(absfe);


		// the projected value is more precise than the quantized value.
		float code_delay_adjustment = (TRACKING_CORRELATOR_SEPARATION * tls->filt_code_error);
		tls->projected_prompt_code_delay = tls->prompt_code_delay +  code_delay_adjustment;


		// Comparator..
		if (tls->filt_code_error > EL_EPSILON) 	 // Positive, shift to left
			tls->prompt_code_delay = addTrackingCodePhases(tls->prompt_code_delay,-code_step);
		else if (tls->filt_code_error < - EL_EPSILON) // Negative, shift to right
			tls->prompt_code_delay = addTrackingCodePhases(tls->prompt_code_delay,code_step);

		// generate early and late code delays from the prompt code delay.
		tls->early_code_delay  = addTrackingCodePhases(tls->prompt_code_delay, 
								-  TRACKING_CORRELATOR_SEPARATION);
		tls->late_code_delay   = addTrackingCodePhases(tls->prompt_code_delay,  
								TRACKING_CORRELATOR_SEPARATION);


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


