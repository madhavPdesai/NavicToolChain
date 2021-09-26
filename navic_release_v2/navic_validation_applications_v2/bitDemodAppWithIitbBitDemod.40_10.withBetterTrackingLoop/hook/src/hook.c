#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <ajit_access_routines.h>
#include <core_portme.h>
#include <app_defines.h>
#include <navic_includes.h>
#include <satellite.h>
#include <bit_demodulate.h>
#include <hook_includes.h>
#include <frame_finder.h>


SatelliteHook sat_hooks[ACTIVE_NUMBER_OF_SATELLITES];
BitDemodFsmState sat_demod_structs[ACTIVE_NUMBER_OF_SATELLITES];
BitDump bit_dump;

extern SatelliteState  volatile  satellite_status[ACTIVE_NUMBER_OF_SATELLITES];
extern uint32_t volatile number_of_interrupts;
FrameFinder global_frame_finder[ACTIVE_NUMBER_OF_SATELLITES];

void initSatelliteHook(SatelliteHook* sh, int sat_id, int sat_band)
{
	initBitDump(&bit_dump);

	sh->sat_id = sat_id;
	sh->band   = sat_band;

	sh->bit_start_ms_count = 0;
	sh->bit_start_code_phase = 0;
	sh->bit_average_code_phase = 0;

	sh->bit_start_doppler = 0;
	sh->bit_average_doppler = 0;

	sh->bit_average_qp = 0;

	sh->has_bit = 0;
	sh->bit_val = 0;

	sh->bit_zero_crossings = 0;

	sh->lost_lock = 0;
}


void initApplicationBody()
{
	int  I;
	for(I=1; I <= ACTIVE_NUMBER_OF_SATELLITES; I++)
	{
		initFrameFinder(&(global_frame_finder[I-1]), I);
		initBitDemodFsmState(&(sat_demod_structs[I-1]), I);

		int rf_band = 
			(I <= 32) ? RF_L1_BAND :
				(I <= 46) ? RF_L5_BAND :
					(I <= 60) ? RF_S_BAND :
						(I <= 63) ? RF_L1_BAND : 0;

		initSatelliteHook (&(sat_hooks[I-1]), I, rf_band);
	}
}

void applicationLoopBody()
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
	satelliteScan();

	//
	// The bits are available after satellite scan..
	//  you can fill your data structure using this
	//  mechanism.
	//
	int i;
	for(i=1; i <= ACTIVE_NUMBER_OF_SATELLITES; i++)
	{
		if(sat_hooks[i-1].has_bit && !sat_hooks[i-1].lost_lock)
		{
			sat_hooks[i-1].has_bit = 0;
			uint8_t bit_val   = sat_hooks[i-1].bit_val;
			uint32_t ms_count = sat_hooks[i-1].bit_start_ms_count;
#ifdef OFFLINE_PRINTF
			PRINTF(":B: %d %d %d %lf %d\n",
					i,
					ms_count,
					sat_hooks[i-1].bit_start_code_phase,
					sat_hooks[i-1].bit_start_doppler,
					bit_val);
#endif
			recordBitInFrameFinder(&(global_frame_finder[i-1]), ms_count, i, bit_val);
			recordBit(&bit_dump, ms_count, i, bit_val);
		}
	}
}

void satelliteScan()
{
	int SAT_ID;
	for(SAT_ID = 1; SAT_ID <= ACTIVE_NUMBER_OF_SATELLITES ; SAT_ID++) 
	{
		SatelliteState* ss = &(satellite_status[SAT_ID-1]);

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
				ss->satellite_progress_word = 0;
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
					int bit_status = 
						injestQp(&(sat_demod_structs[SAT_ID-1]), 
								ms_index, qp,
								(uint32_t) code_phase,
								estimated_doppler,
								&bit_val,
								&bit_ms_count, 
								&bit_code_phase,
								&bit_average_code_phase,
								&bit_doppler,
								&bit_average_doppler,
								&bit_zero_crossings,
								&bit_average_qp);

					if(!bit_status)
					{
						SatelliteHook* sh = &(sat_hooks[SAT_ID-1]);
						if(sh->has_bit)
						{
							PRINTF("Error: Sat %d ms %d hook is not free\n", SAT_ID,
										ms_index);
						}
						else
						{

							sh->bit_start_ms_count = bit_ms_count;
							sh->bit_start_code_phase = bit_code_phase;
							sh->bit_average_code_phase = bit_average_code_phase;
							sh->bit_start_doppler = bit_doppler;
							sh->bit_average_doppler = bit_average_doppler;
							sh->bit_average_qp = bit_average_qp;
							sh->bit_val = bit_val;
							sh->bit_zero_crossings = bit_zero_crossings;		
						
							sh->has_bit = 1;
						}
					}

#ifdef POP_STATUS_PRINT
					//float log_snr_est = 10.0*log10((double) snr_est);
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
	return;
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


void initBitDump(BitDump* bds)
{
	int I;
	for(I = 0; I < 64; I++)
	{
		bds->bit_counter[I] = 0;
		bds->bit_vector[I]  = 0;
	}
}

void recordBit(BitDump* bds, uint32_t ms_index, int ch, int bit)
{
	if((ch > 0) && (ch <= 64))
	{
		int idx = ch-1;

		uint32_t bv = (bds->bit_vector[idx] << 1) | bit;
		bds->bit_counter[idx]++;

		if(bds->bit_counter[idx] == 32)
		{
			PRINTF("BITS %d %d  0x%x\n", ch, ms_index, bv);
			bds->bit_counter[idx] = 0;
			bds->bit_vector[idx]  = 0;
		}
		else
		{
			bds->bit_vector[idx] = bv;
		}
		
	}
}
