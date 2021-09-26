#include <stdlib.h>
#include <stdint.h>
#include <ajit_access_routines.h>
#include <core_portme.h>
#include <math.h>
#include <navic_includes.h>
#include <app_defines.h>
#include <acquire.h>
#include <satellite.h>
#include <hook.h>

extern SatelliteState  volatile  satellite_status[ACTIVE_NUMBER_OF_SATELLITES];
extern uint32_t volatile number_of_interrupts;
extern uint64_t volatile max_ticks_in_interrupt_handler;
extern uint64_t volatile last_ticks_in_interrupt_handler;

void applicationLoopBody()
{
	//
	// scan satellites
	//   extract ip,qp,doppler,phase
	//   values for all satellites being
	//   tracked.  Also gives you lock-lost
	//   information.
	//
	//   pop routine is provided.
	//
	satelliteScan();

	// From the ip/qp values, extract the bits.
	// Plug in your code here.
	
	// Once the frame is built up, analyse it.
	// Plug in your code here.

	// PVT.
	//  use the decoded frames to calculate PVT.
}

void satelliteScan()
{

	// report satellite information.
	int SAT_ID;
	for(SAT_ID = 1; SAT_ID <= ACTIVE_NUMBER_OF_SATELLITES; SAT_ID++)
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
				PRINTF("Satellite %d lock lost\n", ss->sat_id);
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
					PRINTF("MAIN:SAT_%d %d %f %f %d %f %f %f %f\n",
							SAT_ID,
							ms_index, 
							estimated_doppler,
							estimated_phase,
							code_phase, 
							ip, qp,
							10.0*log10(snr_est), 
							lock_val);
				}
			}

		}

	}
	return;
}

int logInterrupt(int last_ic)
{
	if((number_of_interrupts > 0) && (last_ic != number_of_interrupts))
	{
		double max_time_spent = 
			((double) max_ticks_in_interrupt_handler)*1000.0/__CLOCK_FREQUENCY__;
		double last_time_spent = 
			((double) last_ticks_in_interrupt_handler)*1000.0/__CLOCK_FREQUENCY__;
		PRINTF("INTR (%d), max-time-spent=%f last-time-spent=%fms\n", 
				number_of_interrupts, max_time_spent, last_time_spent);
	}
	return(number_of_interrupts);
}

