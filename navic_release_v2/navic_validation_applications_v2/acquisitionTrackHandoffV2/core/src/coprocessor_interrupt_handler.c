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
#include <tracking.h>
#include <satellite.h>
#include <utils.h>

extern NavicState volatile navic_state;
extern uint64_t volatile total_ticks_in_interrupt_handler;
extern uint64_t volatile max_ticks_in_interrupt_handler;
extern uint64_t volatile last_ticks_in_interrupt_handler;
extern uint32_t volatile number_of_interrupts;
int volatile interrupt_flag = 0;

extern float volatile l1_coarse_acquisition_threshold;
extern float volatile l5_coarse_acquisition_threshold;
extern float volatile s_coarse_acquisition_threshold;

extern SatelliteState  volatile  satellite_status[ACTIVE_NUMBER_OF_SATELLITES];

						

float getCoarseAcqThreshold(int rf_band)
{
	return((rf_band == RF_L1_BAND) ? 
			l1_coarse_acquisition_threshold :
				(rf_band == RF_L5_BAND) ? l5_coarse_acquisition_threshold :
					s_coarse_acquisition_threshold);
}

void coprocessor_interrupt_handler()
{
	float ip, qp, ie, qe, il, ql;
	uint64_t t0 = __ajit_get_clock_time();
		
	//PRINTF("INTERRUPT!\n");

	CoprocessorState *cp_state = (CoprocessorState*) &(navic_state.coprocessor_state); 

	// clear the interrupt flag
	writeCoprocessorControlWord (CP_ENABLE_MASK | CP_INTR_ENABLE_MASK | CP_CLEAR_INTR_MASK);


	if(interrupt_flag)
	{
		PRINTF("ERROR: interrupt on interrupt\n");
	}
	interrupt_flag = 1;
	number_of_interrupts++;


	// check responses.
	int sat_index;
	for(sat_index = 0; sat_index < ACTIVE_NUMBER_OF_SATELLITES; sat_index++)
	{
		uint32_t register_value, rf_block_id;
		uint32_t out_args[8];

		SatelliteState* ss = (SatelliteState*) &(satellite_status[sat_index]);

		// ignore satellites that have not been enabled..
		if(!ss->enabled)
			continue;

		
		// probe for command response
		CpCommand op_code;
		CpCommandStatus status = 
			parseCommandResponse(
				(CoprocessorState*) cp_state,
				sat_index,
				&op_code, 
				&register_value, 
				&rf_block_id, 
				out_args);

		// need to detect the end of tracking.
		if(status == COMPLETED)
		{
			clearCommandBuffer(cp_state, sat_index);
			if(op_code == ACQUIRE_OP)
				//
				// acquire can be due to coarse or fine.
				//
			{
				if(getCoarseAcquireStarted(ss))
				{

					setCoarseAcquireStarted(ss,0);

					ss->coarse_acquisition_energy  = (uint32_t) out_args[0];
					ss->coarse_acquisition_doppler = 
						ss->coarse_acquisition_doppler_min  + 
						(ss->coarse_acquisition_doppler_bin_size*
						 ((float) out_args[1]));
					ss->coarse_acquisition_code_phase = 
						addAcquisitionCodePhases 
						(ss->coarse_acquisition_code_phase_min,
						 (out_args[2] *
						  ss->coarse_acquisition_code_phase_step));
					ss->coarse_acquisition_fom = 
						calculateAcquireFom(ss->coarse_acquisition_bin_count,
									out_args[0], out_args[3]);
					

#ifdef DEBUG_PRINTF
					PRINTF("Satellite %d coarse acquire results: E=%f, Doppl = %f, Codephase=%d\n",
							ss->sat_id, 
							(float) ss->coarse_acquisition_energy, 
							ss->coarse_acquisition_doppler, 
							ss->coarse_acquisition_code_phase);
#endif

					if(ss->dummy_satellite == 0) 
					{
						if(ss->coarse_acquisition_energy >= 
								getCoarseAcqThreshold(ss->rf_band))
						{
#ifdef DEBUG_PRINTF
							PRINTF("Satellite %d coarse acquired!\n",
									ss->sat_id);
#endif
							scheduleFineAcquireSatellite(cp_state, ss);
							setFineAcquireStarted(ss,1);
						}
					}
#ifdef DEBUG_PRINTF
					else
					{
						PRINTF("Dummy satellite %d done\n", ss->sat_id);
					}

					PRINTF("Satellite %d progress word after coarse acquire = 0x%x\n", 
							ss->sat_id, ss->satellite_progress_word);

#endif


					// if coarse acquire is done and fine acquire
					// is not started, this indicates that the satellite
					// was rejected.
					setCoarseAcquireDone(ss,1);
				}
				else if(getFineAcquireStarted(ss))
				{
					setFineAcquireStarted(ss,0);
					setFineAcquireDone(ss,1);

					ss->fine_acquisition_energy  = (uint32_t) out_args[0];
					ss->fine_acquisition_doppler = 
						ss->fine_acquisition_doppler_min  + 
						(ss->fine_acquisition_doppler_bin_size*
						 ((float) out_args[1]));
					ss->fine_acquisition_code_phase = 
						addAcquisitionCodePhases
						(ss->fine_acquisition_code_phase_min,
						 (out_args[2] *
						  ss->fine_acquisition_code_phase_step));

#ifdef DEBUG_PRINTF
					PRINTF("Satellite %d fine acquired: E=%f, Doppl = %f, Codephase=%d\n",
							ss->sat_id, 
							(float) ss->fine_acquisition_energy, 
							ss->fine_acquisition_doppler, 
							ss->fine_acquisition_code_phase);
#endif

					schedulePositiveDopplerTrack(cp_state, ss);
					setPositiveDopplerTrackStarted(ss,1);

#ifdef DEBUG_PRINTF
					PRINTF("Satellite %d progress word after fine acquire = 0x%x\n", 
							ss->sat_id, ss->satellite_progress_word);
#endif
				}
			}
			else if(op_code == TRACK_OP)
			{
				getTrackResults (out_args, &ie, &qe, &ip, &qp, &il, &ql);
				if(getPositiveDopplerTrackStarted(ss))
				{
					setPositiveDopplerTrackStarted(ss,0);
					setPositiveDopplerTrackDone(ss,1);

					ss->positive_doppler_track_energy = sqrt((ip*ip) + (qp*qp));
#ifdef DEBUG_PRINTF
					PRINTF("Satellite %d positive doppler ip=%f, qp=%f, energy = %f\n", 
							ss->sat_id,
							ip, qp,
							ss->positive_doppler_track_energy);
#endif

					scheduleNegativeDopplerTrack(cp_state, ss);
					setNegativeDopplerTrackStarted(ss,1);
#ifdef DEBUG_PRINTF
					PRINTF("Satellite %d progress word after positive doppler track = 0x%x\n", 
							ss->sat_id, ss->satellite_progress_word);
#endif
				}
				else if(getNegativeDopplerTrackStarted(ss))
				{
					setNegativeDopplerTrackStarted(ss,0);
					setNegativeDopplerTrackDone(ss,1);

					ss->negative_doppler_track_energy = sqrt((ip*ip) + (qp*qp));
#ifdef DEBUG_PRINTF
					PRINTF("Satellite %d negative doppler ip=%f, qp=%f, energy = %f\n", 
							ss->sat_id,
							ip, qp,
							ss->negative_doppler_track_energy);
#endif

					ss->tls.loop_count = 0;	
					scheduleNormalTracking(cp_state, ss);
					setTrackingStarted(ss, 1);
#ifdef DEBUG_PRINTF
					PRINTF("Satellite %d progress word after negative doppler track = 0x%x\n", 
							ss->sat_id, ss->satellite_progress_word);
#endif
				}
				else if(getTrackingStarted(ss) && !getTrackLost(ss))
				{

					// record the track information.
					pushIntoTrackHistoryQueue(&(ss->sthsq),
							number_of_interrupts,
							ss->tls.carrier_frequency - 
								getTrackIf(ss->rf_band),
							ss->tls.accumulated_phase,
							ss->tls.prompt_code_delay,
							ip, qp,
							ss->tls.snr_estimate,
							ss->tls.lock_value);


					// execute tracking loop iteration.
					runTrackingLoopIteration(ie, qe, ip, qp, il, ql, 
							&(ss->tls));
#ifdef DEBUG_PRINTF
					PRINTF("ISR:SAT_%d %d %f %f %f %f %d %f %f %f\n",
							ss->sat_id,
							ss->tls.loop_count, 
							ss->tls.carrier_frequency,
							ss->tls.carrier_phase,
							ss->tls.theta, 
							ss->tls.accumulated_phase,
							ss->tls.prompt_code_delay,
							ip, qp, ss->tls.P);
#endif

					// schedule track operation
					if(ss->tls.lock_status)
					{
						scheduleNormalTracking(cp_state, ss);
					}
					else
					{
						//
						// track lost.. no further actions pending
						//
						setTrackLost(ss, 1);	
#ifdef DEBUG_PRINTF
						PRINTF("ISR:SAT_%d track lost at time %d\n",
								ss->sat_id, number_of_interrupts);
#endif
					}
				}
			}
		}
	}

	// generate command strobe
	writeCoprocessorControlWord (CP_ENABLE_MASK | CP_INTR_ENABLE_MASK | 
			CP_COMMAND_STROBE_MASK);

	// timing.
	uint64_t t1 = __ajit_get_clock_time();
	uint64_t delta_t = (t1 - t0);

	if(max_ticks_in_interrupt_handler < delta_t)
		max_ticks_in_interrupt_handler = delta_t;

	last_ticks_in_interrupt_handler   = delta_t;
	total_ticks_in_interrupt_handler += delta_t;

	// return..
	interrupt_flag = 0;
	return;
}


