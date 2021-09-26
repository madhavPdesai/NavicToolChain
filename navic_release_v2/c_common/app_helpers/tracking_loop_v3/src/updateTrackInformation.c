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
//#include <navic_includes.h>
#include <tracking.h>
#include <app_defines.h>
#include <utils.h>
#include <stdio.h>


//#define __QUANTIZE__(x) ((x < 0.5) ? 1 : 3)
//#define __QUANTIZE__(x) ((x < 0.5) ? 1 : 3)
//#define __QUANTIZE__(x) ((x > EL_EPSILON) ? 1 : 0)

//
// quantize by truncating using 4 values.
//
#define __QUANTIZE__(x) ((x < 0.25) ? 1 : ((x < 0.5) ? 2 : ((x < 0.75)  ? 3 : 4)))

#define RPHASE_SCALE_FACTOR (2.0*M_PI*0.001)
#define THETA_SCALE_FACTOR  (1000.0/(2*M_PI))

int runTrackingLoopIteration(
		double ie, 
		double qe,
		double ip,
		double qp,
		double il,
		double ql,
		TrackingLoopState* tls
		) 
{

	tls->ip_a += ip;
	tls->qp_a += qp;
	tls->ie_a += ie;
	tls->qe_a += qe;
	tls->il_a += il;
	tls->ql_a += ql;
	tls->carrier_track_counter++;

	tls->cp_ie_a += ie;
	tls->cp_qe_a += qe;
	tls->cp_il_a += il;
	tls->cp_ql_a += ql;
	tls->code_phase_track_counter++;

	// residual carrier phase is always updated to keep the NCO current.
	double residual_phase = (tls->carrier_frequency*RPHASE_SCALE_FACTOR);
	tls->accumulated_residual_phase = normalizeTo2Pi(tls->accumulated_residual_phase + residual_phase);

	if(tls->carrier_track_counter == tls->carrier_integration_interval_in_ms)
	{

		double r = (tls->ip_a/(tls->d_offset + tls->qp_a));

		// TODO: replace this with a table lookup.
		tls->theta  = atan(r);

		// frequency tracking loop.
		tls->freq_dev       = tls->theta*THETA_SCALE_FACTOR;
		tls->carrier_frequency = (tls->k1_f*tls->freq_dev) + (tls->k2_f*tls->carrier_frequency);

		// phase tracking loop.
		tls->accumulated_phase = (tls->k1_p*tls->theta) + (tls->k2_p*tls->accumulated_phase);
		tls->carrier_phase = normalizeTo2Pi(tls->accumulated_residual_phase + tls->accumulated_phase);

		tls->carrier_track_counter = 0;
		tls->ip_a = 0;
		tls->qp_a = 0;
		tls->ie_a = 0;
		tls->qe_a = 0;
		tls->il_a = 0;
		tls->ql_a = 0;
	}
	else
	{
		tls->carrier_phase = normalizeTo2Pi(tls->carrier_phase + residual_phase);
	}

	if(tls->code_phase_track_counter == tls->code_phase_integration_interval_in_ms)
	{

		///------------------- Code Tracking Loop ----------------------------
		// Calculate the Envelope values
		double E = ((tls->cp_ie_a*tls->cp_ie_a)+(tls->cp_qe_a*tls->cp_qe_a));
		double L = ((tls->cp_il_a*tls->cp_il_a)+(tls->cp_ql_a*tls->cp_ql_a));

		double EmL = E - L;

		//
		//------------------------- code phase DLL -----------------------
		//
		double filt_code_error_prev = tls->filt_code_error;
		tls->code_error = EmL / (E + L);
		tls->filt_code_error = (tls->k1_c*tls->code_error) +  (tls->k2_c*tls->filt_code_error);

		if ((tls->filt_code_error * filt_code_error_prev) >= 0) // No zero crossing
		{
			if (tls->filt_code_error > 0) 	 // Positive, shift to left
				tls->prompt_code_delay = addTrackingCodePhases(tls->prompt_code_delay,-1);
			else if (tls->filt_code_error < 0) // Negative, shift to right
				tls->prompt_code_delay = addTrackingCodePhases(tls->prompt_code_delay,1);
		}

		// generate early and late code delays from the prompt code delay.
		tls->early_code_delay  = addTrackingCodePhases(tls->prompt_code_delay, 
				-  TRACKING_CORRELATOR_SEPARATION);
		tls->late_code_delay   = addTrackingCodePhases(tls->prompt_code_delay,  
				TRACKING_CORRELATOR_SEPARATION);


		tls->code_phase_track_counter = 0;
		tls->cp_ie_a = 0;
		tls->cp_qe_a = 0;
		tls->cp_il_a = 0;
		tls->cp_ql_a = 0;
	}
		


	updateLockAndCnrInformation(ip,qp,tls);

	tls->loop_count += 1;
	return(tls->lock_status);
}

void updateLockAndCnrInformation(double ip, double qp, TrackingLoopState* tls)
{
	tls->qp_sign_changes += ((qp * tls->last_qp) < 0);
	tls->last_qp = qp;

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
				((tls->qp_sign_changes < 4) && 
				 (tls->lock_value > LOCK_THRESHOLD) &&
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
		tls->qp_sign_changes = 0;
	}

	tls->ip2_sum += ip*ip;
	tls->qp2_sum += qp*qp;

	tls->lock_counter++;
}


