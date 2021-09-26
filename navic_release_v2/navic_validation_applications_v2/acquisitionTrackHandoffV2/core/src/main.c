//
// An application which initializes the coprocessor and rf resampler
//
//
#include <stdlib.h>
#include <stdint.h>
#include <ajit_access_routines.h>
#include <core_portme.h>
#include <math.h>
#include <navic_includes.h>
#include <app_defines.h>
#include <acquire.h>
#include <satellite.h>
#include <hook_includes.h>


uint64_t volatile total_ticks_in_interrupt_handler = 0;
uint64_t volatile max_ticks_in_interrupt_handler = 0;
uint64_t volatile last_ticks_in_interrupt_handler = 0;
uint32_t volatile number_of_interrupts = 0;

float volatile l1_coarse_acquisition_threshold=ACQUISITION_THRESHOLD;
float volatile l5_coarse_acquisition_threshold=ACQUISITION_THRESHOLD;
float volatile s_coarse_acquisition_threshold=ACQUISITION_THRESHOLD;

NavicState volatile navic_state;
SatelliteState  volatile satellite_status[ACTIVE_NUMBER_OF_SATELLITES];

// defined in src/init_soc.c
void init_soc();
void initApplicationBody();

	
			
void markSatellitesForReacquire(CoprocessorState* cp_state)
{
	int SAT_ID;
	for(SAT_ID = 1; SAT_ID <= ACTIVE_NUMBER_OF_SATELLITES; SAT_ID++)
	{
		SatelliteState* ss = (SatelliteState*) &(satellite_status[SAT_ID-1]);
		if(ss->enabled && getTrackLost(ss))
		{
			ss->satellite_progress_word = 0;

			ss->tls.carrier_integration_interval_in_ms = TRACK_NMS;
			ss->tls.code_phase_integration_interval_in_ms = TRACK_NMS;

			scheduleCoarseAcquireSatellite(cp_state, ss);
			setCoarseAcquireStarted(ss,1);
			PRINTF("Info: scheduled re-acquire for satellite %d. \n", ss->sat_id);
		}
	}
}


void printSatelliteAcquireStats()
{
	int SAT_ID;
	for(SAT_ID = 1; SAT_ID <= ACTIVE_NUMBER_OF_SATELLITES; SAT_ID++)
	{
		SatelliteState* ss = (SatelliteState*) &(satellite_status[SAT_ID-1]);
		PRINTF("Satellite %d coarse acquire results: E=%f (FOM=%f), Doppl = %f, Codephase=%d\n",
					ss->sat_id, 
					(float) ss->coarse_acquisition_energy, 
					ss->coarse_acquisition_fom,
					ss->coarse_acquisition_doppler, 
					ss->coarse_acquisition_code_phase);
		PRINTF("Satellite %d fine acquire results: E=%f, Doppl = %f, Codephase=%d\n",
					ss->sat_id, 
					(float) ss->fine_acquisition_energy, 
					ss->fine_acquisition_doppler, 
					ss->fine_acquisition_code_phase);
		
		PRINTF("Satellite %d tracking loop integration intervals=%d,%d\n", SAT_ID, 
				ss->tls.carrier_integration_interval_in_ms,
				ss->tls.carrier_integration_interval_in_ms);
	}
}


//
// 
// 
int main()
{

	// lots of initialization.
	init_soc();

	// application side..
	initApplicationBody();

	PRINTF(" options: a for stop-after 1024 ms\nb for stop-after 2048 ms\nc for run-till lock-lost\n");
	char c;
	while(1)
	{
		c = __ajit_serial_getchar__();
		if(c != 0)
			break;
	}

	//
	// nominal run limit.
	// 2 hours.
	//
	int run_limit = 2 * 3600 * 1000;

	if (c == 'a')
		run_limit = 32*1024;
	else if (c == 'b')
		run_limit = 64*2048;
	
	
	PRINTF("Info: run_limit=%d.\n", run_limit);

	// coprocessor state handle.
	CoprocessorState* cp_state = (CoprocessorState*) &(navic_state.coprocessor_state);

	// estimate acquisition thresholds	
#if GPS_L1
	PRINTF("Info: estimating L1 acquire threshold\n");
	l1_coarse_acquisition_threshold = estimateAcquisitionThreshold(cp_state, RF_L1_BAND);
	PRINTF("Info: L1 acquire threshold = %f\n", l1_coarse_acquisition_threshold);
#endif
#if IRNSS_L5
	PRINTF("Info: estimating L5 acquire threshold\n");
	l5_coarse_acquisition_threshold = estimateAcquisitionThreshold(cp_state, RF_L5_BAND);
	PRINTF("Info: L5 acquire threshold = %f\n", l5_coarse_acquisition_threshold);
#endif
#if IRNSS_S
	PRINTF("Info: estimating S acquire threshold\n");
	s_coarse_acquisition_threshold = estimateAcquisitionThreshold(cp_state, RF_S_BAND);
	PRINTF("Info: S acquire threshold = %f\n", s_coarse_acquisition_threshold);
#endif

	uint32_t f2_quant_threshold = getFilterQuantizationThreshold (RF_L1_BAND, 2);
	uint32_t f3_quant_threshold = getFilterQuantizationThreshold (RF_L1_BAND, 3);
	PRINTF("Info: quantization threshold filter_2=%d, filter_3=%d.\n", f2_quant_threshold, f3_quant_threshold);



	// for keeping track of interrupts.  last interrupt counter value
	// seen by logInterrupt.
	int last_ic = -1;
	int I;


	int SAT_ID;
	for(SAT_ID = 1; SAT_ID <= ACTIVE_NUMBER_OF_SATELLITES; SAT_ID++)
	//
	// Schedule the satellites for acquisition, one at a time!  
	// This is because each satellite goes through a sequence of
	//   coarse-acquisition
	//   fine-acquisition
	//   positive doppler track operation
	//   negative doppler track operation
	// before the tracking of the satellite starts.
	//
	// There is a single queue for the acquire engine.
	// If we start all coarse acquires in parallel, the
	// fine acquire of a particular satellite will not 
	// start until all the coarse acquisitions are completed.
	// And this is not what we want.
	//
	// So we schedule coarse acquire for a satellite, and 
	// schedule the next coarse acquire only after the first
	// coarse-acquire has completed.
	//
	{
		SatelliteState* ss = (SatelliteState*) &(satellite_status[SAT_ID-1]);
		if(ss->enabled && !ss->dummy_satellite)
		{
			setTrackingLoopParameters(&(ss->tls), LOCK_COMPUTATION_INTERVAL,
				DOFFSET, K1_F, K2_F, K1_P, K2_P, K1_C, K2_C);

			// schedule
			scheduleCoarseAcquireSatellite(cp_state, ss);
			setCoarseAcquireStarted(ss,1);
			PRINTF("Info: scheduled acquire for satellite %d. \n", ss->sat_id);

			//
			// while waiting for SAT_ID coarse acquisition to
			// complete, you can start monitoring ongoing tracks
			// using applicationLoopBody.
			//
			while(!getCoarseAcquireDone(ss))
			{
				last_ic = logInterrupt(last_ic);
			
				// application will be invoked.
				int rv = applicationLoopBody();
				if(rv)
				{
					PRINTF("Info: some satellite lost lock immediately after acquire...\n");
					printSatelliteHookStats();
					return(0);
				}
			}

			PRINTF("Satellite %d coarse acquire results: E=%f (FOM=%f), Doppl = %f, Codephase=%d\n",
					ss->sat_id, 
					(float) ss->coarse_acquisition_energy, 
					ss->coarse_acquisition_fom,
					ss->coarse_acquisition_doppler, 
					ss->coarse_acquisition_code_phase);

		}

	}


	// PVT loop.
	last_ic    = -1;
	while(1)
	{
		last_ic = logInterrupt(last_ic);

		//
		// The application body.
		//
		int ret_val = applicationLoopBody();
	
		if(ret_val)
		{
				//markSatellitesForReacquire(cp_state);
				break;
		}

		if((number_of_interrupts & 0x3ff) == 0)
			PRINTF("Finished %d: app body.\n", number_of_interrupts);

		if(number_of_interrupts > run_limit)
		{
			PRINTF("Reached run limit %d.\n", run_limit);
			break;
		}
			
	}

	printSatelliteAcquireStats();
	printSatelliteHookStats();

	return (0);
}


