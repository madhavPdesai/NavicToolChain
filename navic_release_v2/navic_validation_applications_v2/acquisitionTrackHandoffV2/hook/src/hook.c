#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <ajit_access_routines.h>
#include <core_portme.h>
#include <app_defines.h>
#include <navic_includes.h>
#include <satellite.h>
#include <hook_includes.h>


SatelliteHook sat_hooks[ACTIVE_NUMBER_OF_SATELLITES];
extern SatelliteState  volatile  satellite_status[ACTIVE_NUMBER_OF_SATELLITES];
extern uint32_t volatile number_of_interrupts;

void initSatelliteHook(SatelliteHook* sh, int sat_id, int sat_band)
{
	sh->sat_id = sat_id;
	sh->band   = sat_band;

	sh->write_pointer = 0;

	int I;
	for(I = 0; I < QSIZE; I++)
	{
		sh->ip[I] = 0.0;
		sh->qp[I] = 0.0;
		sh->doppler[I] = 0.0;
		sh->code_phase[I] = 0;
		sh->ms_count[I] = 0;
		sh->lock_val[I] = 0.0;
		sh->snr_estimate[I] = 0.0;
	}

	sh->lost_lock = 0;
}

void absorbQp(  SatelliteHook* sh,
		uint32_t ms_count, 
		float ip, 
		float qp, 
		uint32_t code_phase,
		float doppler,
		float snr_est,
		float lock_val)
{
	int I = sh->write_pointer;

	sh->ip[I] = ip;
	sh->qp[I] = qp;
	sh->doppler[I] = doppler;
	sh->code_phase[I] = code_phase;
	sh->ms_count[I] = ms_count;
	sh->lock_val[I] = lock_val;
	sh->snr_estimate[I] = snr_est;
	
	
	sh->write_pointer = (I + 1) & QSIZE_MASK;
}


void printSatelliteHookStats()
{
	int I;
	for(I = 0; I < ACTIVE_NUMBER_OF_SATELLITES; I++)
	{
		printSatelliteHook(&(sat_hooks[I]));
	}
}

void printSatelliteHook(SatelliteHook* sh)
{	
	PRINTF("Satellite hook summary: sat=%d, band=%d.\n", sh->sat_id, sh->band);

	int I;
	for (I = 1; I <= QSIZE; I++)
	{
		int P = (sh->write_pointer + I) & QSIZE_MASK;
		PRINTF("   %d %d  %f %f %d %f %f %f\n", I, sh->ms_count[P], sh->ip[P], sh->qp[P], sh->code_phase[I], sh->doppler[I], sh->lock_val[I], sh->snr_estimate[I]);
	}
}

void initApplicationBody()
{
	int  I;
	for(I=1; I <= ACTIVE_NUMBER_OF_SATELLITES; I++)
	{
		int rf_band = 
			(I <= 32) ? RF_L1_BAND :
				(I <= 46) ? RF_L5_BAND :
					(I <= 60) ? RF_S_BAND :
						(I <= 63) ? RF_L1_BAND : 0;

		initSatelliteHook (&(sat_hooks[I-1]), I, rf_band);
	}
}

int applicationLoopBody()
{
	//
	// scan satellites
	//   extract ip,qp,doppler,phase
	//   values for all satellites being
	//   tracked.  Also gives you lock-lost
	//   information.
	//
	//   Push the information to bit demod 
	//   and update SatelliteHook.
	//
	int ret_val = satelliteScan();
	return(ret_val);
}

int satelliteScan()
{
	int SAT_ID;
	int ret_val = 0;

	for(SAT_ID = 1; SAT_ID <= ACTIVE_NUMBER_OF_SATELLITES ; SAT_ID++) 
	{
		SatelliteState* ss = &(satellite_status[SAT_ID-1]);
		SatelliteHook* sh = &(sat_hooks[SAT_ID-1]);
		

		// if not enabled, continue.
		if(!ss->enabled || ss->dummy_satellite)
			continue;

		// This is the hook for the user application to pull out
		// information for the upper layer actions.
		if(getTrackingStarted(ss))
		{
			// track lost? stop reporting.
			if(getTrackLost(ss))
			{
				PRINTF("Satellite %d lock lost (lock_value=%f,snr=%f).\n", ss->sat_id, ss->tls.lock_value, ss->tls.snr_estimate);
				// reset the satellite progress word.
				ss->satellite_progress_word = TRACK_LOST_MASK;
				sh->lost_lock = 1;
				ret_val = 1;
				continue;
			}

			int n_entries = numberOfEntriesInTrackHistoryQueue(&(ss->sthsq));

			if(n_entries > 0)
			{
				int32_t ms_index;	

				// code phase measurement units are 1/16 chip
				int32_t code_phase;	

				float estimated_doppler, estimated_phase;
				float ip, qp, snr_est, lock_val;

				// disable interrupts else you could have
				// a mutex-induced deadlock.
				__AJIT_WRITE_IRC_CONTROL_REGISTER__(0);

				//
				// sacrificial..  if interrupt occurs, there is
				// enough time spent here before it sneaks into pop
				//
				__ajit_sleep__(16);

				int pop_ok = popFromTrackHistoryQueue(&(ss->sthsq),
						&ms_index,
						&estimated_doppler,
						&estimated_phase,
						&code_phase,
						&ip, &qp,
						&snr_est, &lock_val);

				__AJIT_WRITE_IRC_CONTROL_REGISTER__(1);

				if(pop_ok == 0)
				{
					uint8_t bit_val;
					uint32_t bit_ms_count;
					uint32_t bit_code_phase;
					double  bit_average_code_phase;
					double  bit_doppler, bit_average_doppler;
					uint8_t bit_zero_crossings;
					double  bit_average_qp;

					// lets injest the qp values and
					// re-confirm if bits are ok.
					absorbQp(sh, ms_index, ip, qp, (uint32_t) code_phase, estimated_doppler, snr_est, lock_val);

#ifdef POP_STATUS_PRINT
					PRINTF("MAIN:SAT_%d %d %f %f %d %f %f %f %f\n",
							SAT_ID,
							ms_index, 
							estimated_doppler,
							estimated_phase,
							code_phase, 
							ip, qp,
							snr_est, // log_snr_est, //10.0*log10(snr_est), 
							lock_val);
#endif
				}

			}

		}

	}
	return (ret_val);
}

int logInterrupt(int last_ic)
{
#ifdef DEBUG_PRINTF
	if((number_of_interrupts > 0) && (last_ic != number_of_interrupts))
	{
		double max_time_spent = 
			((double) max_ticks_in_interrupt_handler)*1000.0/__CLOCK_FREQUENCY__;
		double last_time_spent = 
			((double) last_ticks_in_interrupt_handler)*1000.0/__CLOCK_FREQUENCY__;
		PRINTF("INTR (%d), max-time-spent=%f last-time-spent=%fms\n", 
				number_of_interrupts, max_time_spent, last_time_spent);
	}
#endif
	return(number_of_interrupts);
}

