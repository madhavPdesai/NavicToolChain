#include <stdint.h>
#include <stdlib.h>
#include <ajit_access_routines.h>
#include <core_portme.h>
#include <math.h>
#include <navic_includes.h>
#include <app_defines.h>
#include <satellite.h>
#include <pthread.h>
#include <utils.h>

extern SatelliteState  volatile  satellite_status[ACTIVE_NUMBER_OF_SATELLITES];


//
// Satellite number 1 to 32: GPS, L1 band.
// Satellite number 33 to 46:  IRNSS satellites 1 to 14, L5 band.
// Satellite number 47 to 60:  IRNSS satellites 15 to 28, S band.
// Satellite number 61: Gagan
// Satellite number 62: Gagan
// Satellite number 63: Gagan
// Satellite number 64: Dummy
//
void initializeAllSatelliteStructures(int gps_l1, int irnss_l5, int irnss_s)
{
	int SAT_ID;
	for(SAT_ID = 1; SAT_ID <= ACTIVE_NUMBER_OF_SATELLITES; SAT_ID++)
	{
		SatelliteState* ss = (SatelliteState*) &(satellite_status[SAT_ID-1]);
		int enable_flag = 0;
		int dummy_flag  = 0;

		int rf_band;
		if(SAT_ID <= 32)
		{
			rf_band = RF_L1_BAND;
			enable_flag = gps_l1;
		}
		else if(SAT_ID <= 46)
		{
			rf_band = RF_L5_BAND;
			enable_flag = irnss_l5;
		}
		else if(SAT_ID <= 60)
		{
			rf_band = RF_S_BAND;
			enable_flag = irnss_s;
		}
		else if((SAT_ID >= 61) && (SAT_ID <= 63))
		// GAGAN satellites are not enabled.
		{
			rf_band = RF_L1_BAND;
			dummy_flag = 0;
			enable_flag = 0;
		}
		else
		{
			//
			// this field will keep changing.
			//
			rf_band = RF_S_BAND;


			dummy_flag = 1;
			enable_flag = 0;
		}
		initSatelliteState(ss, SAT_ID, rf_band, dummy_flag, enable_flag);
	}
}


void initSatelliteState(SatelliteState* ss, int sat_id, int rf_band, int dummy_flag, int enable_flag)
{
	ss->sat_id = sat_id;
	ss->rf_band = rf_band;
	ss->enabled = enable_flag;

	//
	// set this if only coarse acq is to
	// be done..
	//
	ss->dummy_satellite = dummy_flag;

	ss->coarse_acquisition_doppler = 0;
	ss->coarse_acquisition_code_phase = 0;
	ss->coarse_acquisition_energy = 0;

	ss->coarse_acquisition_code_phase_min = 0;
	ss->coarse_acquisition_code_phase_max = 0;
	ss->coarse_acquisition_code_phase_step = 0;

	ss->coarse_acquisition_doppler_min = 0;
	ss->coarse_acquisition_doppler_max = 0;
	ss->coarse_acquisition_doppler_bin_size = 0;
	
	ss->fine_acquisition_doppler = 0;
	ss->fine_acquisition_code_phase = 0;
	ss->fine_acquisition_energy = 0;

	ss->fine_acquisition_code_phase_min = 0;
	ss->fine_acquisition_code_phase_max = 0;
	ss->fine_acquisition_code_phase_step = 0;

	ss->fine_acquisition_doppler_min = 0;
	ss->fine_acquisition_doppler_max = 0;
	ss->fine_acquisition_doppler_bin_size = 0;

	ss->positive_doppler_track_energy = 0;
	ss->negative_doppler_track_energy = 0;

	ss->satellite_progress_word = 0;

	initTrackingLoopState(0,0,getTrackIf(ss->rf_band), 0,0,&(ss->tls));
	initTrackHistoryQueue(&(ss->sthsq));
}

void scheduleDopplerTrackBase(CoprocessorState* cp_state, SatelliteState* ss, 
					double carrier_frequency)
{
	double carrier_phase = 0;
	int32_t prompt_code_phase  = 
		ss->fine_acquisition_code_phase * 
			(TRACKING_SAMPLES_PER_CODE/ACQUISITION_SAMPLES_PER_CODE) ;

	int32_t early_code_phase = 
		addTrackingCodePhases(ss->fine_acquisition_code_phase,  
						-TRACKING_SAMPLES_PER_HALF_CODE_CHIP);
	int32_t late_code_phase  = addTrackingCodePhases(ss->fine_acquisition_code_phase,   
						TRACKING_SAMPLES_PER_HALF_CODE_CHIP);

#ifdef DEBUG_PRINTF
	PRINTF("ScheduleTrack: positive doppler:  freq=%f, code-phase=%d\n",
			carrier_frequency, 
			prompt_code_phase);
#endif

	scheduleTrackCommand(cp_state,
				ss->sat_id - 1,
				ss->rf_band, 
				(double) carrier_frequency,
				(double) carrier_phase,
				early_code_phase,
				prompt_code_phase,
				late_code_phase
				);

}

void schedulePositiveDopplerTrack(CoprocessorState* cp_state, SatelliteState* ss)
{
	float track_if = getTrackIf(ss->rf_band);
	double carrier_frequency = 
		track_if + ss->fine_acquisition_doppler;
	scheduleDopplerTrackBase(cp_state, ss, carrier_frequency);
}

void scheduleNegativeDopplerTrack(CoprocessorState* cp_state, SatelliteState* ss)
{
	double carrier_frequency = getTrackIf(ss->rf_band) - ss->fine_acquisition_doppler;
	scheduleDopplerTrackBase(cp_state, ss, carrier_frequency);
}

void scheduleNormalTracking(CoprocessorState* cp_state, 
					SatelliteState* ss)
{
	if(ss->tls.loop_count == 0)
	{
		double carrier_frequency = 
			(ss->positive_doppler_track_energy > ss->negative_doppler_track_energy)
				? (getTrackIf(ss->rf_band) + ss->fine_acquisition_doppler)
				:  (getTrackIf(ss->rf_band) - ss->fine_acquisition_doppler);

		initTrackingLoopState(
				TRACK_NMS,
				TRACK_NMS,
				ss->fine_acquisition_code_phase * 
					(TRACKING_SAMPLES_PER_CODE/ACQUISITION_SAMPLES_PER_CODE) ,
				carrier_frequency,
				0.0,
				&(ss->tls));	
	}

	scheduleTrackCommand(cp_state,
				ss->sat_id - 1,
				ss->rf_band, 
				ss->tls.carrier_frequency,
				ss->tls.carrier_phase,
				ss->tls.early_code_delay,
				ss->tls.prompt_code_delay,
				ss->tls.late_code_delay
				);
}

void setCoarseAcquireStarted(SatelliteState* ss, int val)
{
	ss->satellite_progress_word = 
		(val ? (COARSE_ACQUIRE_STARTED_MASK | ss->satellite_progress_word) :
			((~ COARSE_ACQUIRE_STARTED_MASK) & ss->satellite_progress_word));
		
}
int  getCoarseAcquireStarted(SatelliteState* ss)
{
	return((COARSE_ACQUIRE_STARTED_MASK & ss->satellite_progress_word) != 0);
}

void setCoarseAcquireDone(SatelliteState* ss, int val)
{
	ss->satellite_progress_word = 
		(val ? (COARSE_ACQUIRE_DONE_MASK | ss->satellite_progress_word) :
			((~ COARSE_ACQUIRE_DONE_MASK) & ss->satellite_progress_word));
}
int  getCoarseAcquireDone(SatelliteState* ss)
{
	return((COARSE_ACQUIRE_DONE_MASK & ss->satellite_progress_word) != 0);
}

void setFineAcquireStarted(SatelliteState* ss, int val)
{
	ss->satellite_progress_word = 
		(val ? (FINE_ACQUIRE_STARTED_MASK | ss->satellite_progress_word) :
			((~ FINE_ACQUIRE_STARTED_MASK) & ss->satellite_progress_word));
}

int  getFineAcquireStarted(SatelliteState* ss)
{
	return((FINE_ACQUIRE_STARTED_MASK & ss->satellite_progress_word) != 0);
}

void setFineAcquireDone(SatelliteState* ss, int val)
{
	ss->satellite_progress_word = 
		(val ? (FINE_ACQUIRE_DONE_MASK | ss->satellite_progress_word) :
			((~ FINE_ACQUIRE_DONE_MASK) & ss->satellite_progress_word));
}
int  getFineAcquireDone(SatelliteState* ss)
{
	return((FINE_ACQUIRE_DONE_MASK & ss->satellite_progress_word) != 0);
}


void setPositiveDopplerTrackStarted(SatelliteState* ss, int val)
{
	ss->satellite_progress_word = 
		(val ? (POSITIVE_DOPPLER_TRACK_STARTED_MASK | ss->satellite_progress_word) :
			((~ POSITIVE_DOPPLER_TRACK_STARTED_MASK) & ss->satellite_progress_word));
}
int  getPositiveDopplerTrackStarted(SatelliteState* ss)
{
	return((POSITIVE_DOPPLER_TRACK_STARTED_MASK & ss->satellite_progress_word) != 0);
}

void setPositiveDopplerTrackDone(SatelliteState* ss, int val)
{
	ss->satellite_progress_word = 
		(val ? (POSITIVE_DOPPLER_TRACK_DONE_MASK | ss->satellite_progress_word) :
			((~ POSITIVE_DOPPLER_TRACK_DONE_MASK) & ss->satellite_progress_word));
}
int  getPositiveDopplerTrackDone(SatelliteState* ss)
{
	return((POSITIVE_DOPPLER_TRACK_DONE_MASK & ss->satellite_progress_word) != 0);
}

void setNegativeDopplerTrackStarted(SatelliteState* ss, int val)
{
	ss->satellite_progress_word = 
		(val ? (NEGATIVE_DOPPLER_TRACK_STARTED_MASK | ss->satellite_progress_word) :
			((~ NEGATIVE_DOPPLER_TRACK_STARTED_MASK) & ss->satellite_progress_word));
}
int  getNegativeDopplerTrackStarted(SatelliteState* ss)
{
	return((NEGATIVE_DOPPLER_TRACK_STARTED_MASK & ss->satellite_progress_word) != 0);
}

void setNegativeDopplerTrackDone(SatelliteState* ss, int val)
{
	ss->satellite_progress_word = 
		(val ? (NEGATIVE_DOPPLER_TRACK_DONE_MASK | ss->satellite_progress_word) :
			((~ NEGATIVE_DOPPLER_TRACK_DONE_MASK) & ss->satellite_progress_word));
}
int  getNegativeDopplerTrackDone(SatelliteState* ss)
{
	return((NEGATIVE_DOPPLER_TRACK_DONE_MASK & ss->satellite_progress_word) != 0);
}

void setTrackingStarted(SatelliteState* ss, int val)
{
	ss->satellite_progress_word = 
		(val ? (TRACK_STARTED_MASK | ss->satellite_progress_word) :
			((~ TRACK_STARTED_MASK) & ss->satellite_progress_word));
}
int  getTrackingStarted(SatelliteState* ss)
{
	return((TRACK_STARTED_MASK & ss->satellite_progress_word) != 0);
}

void setTrackLost(SatelliteState* ss, int val)
{
	ss->satellite_progress_word = 
		(val ? (TRACK_LOST_MASK | ss->satellite_progress_word) :
			((~ TRACK_LOST_MASK) & ss->satellite_progress_word));
}
int  getTrackLost(SatelliteState* ss)
{
	return((TRACK_LOST_MASK & ss->satellite_progress_word) != 0);
}

void initTrackHistoryQueue (SatelliteTrackHistoryQueue* sthq)
{
	sthq->write_pointer = 0;
	sthq->read_pointer  = 0;
	sthq->iq_mutex = 0;
}

// spin until you get mutex

void minimal_acquire_mutex_using_swap();
uint32_t  __ajit_swap_asm__(uint32_t loc, uint32_t val);

void acquireMutexForTrackHistoryQueue(SatelliteTrackHistoryQueue* sthq)
{
	uint32_t val = 1;
	while(1)
	{
		val = 1;
		val = __ajit_swap_asm__((uint32_t) &(sthq->iq_mutex), val);
		if(!val) break;
	
		while(sthq->iq_mutex)
		{
			__ajit_sleep__(256);
		}
	}
}

void minimal_release_mutex_using_swap();
void releaseMutexForTrackHistoryQueue(SatelliteTrackHistoryQueue* sthq)
{
	sthq->iq_mutex = 0;
}


int  pushIntoTrackHistoryQueue(SatelliteTrackHistoryQueue* sthq,
					uint32_t ms_count,
					float f, float p,
					int32_t prompt_code_phase,
					float ip, float qp,
					float snr_estimate,
					float lock_value)
{
	acquireMutexForTrackHistoryQueue(sthq);

	uint32_t wp = sthq->write_pointer;
	uint32_t rp = sthq->read_pointer;

	sthq->estimated_doppler[wp] = f;
	sthq->estimated_phase[wp] = p;
	sthq->ip[wp] = ip;
	sthq->qp[wp] = qp;
	sthq->ms_count[wp] = ms_count;

	sthq->prompt_code_phase[wp] = prompt_code_phase;

	sthq->write_pointer = (wp + 1) & IQ_SAMPLE_DEPTH_MASK;
	// overrun.
	if(sthq->write_pointer == rp)
		sthq->read_pointer = (rp + 1) & IQ_SAMPLE_DEPTH_MASK;

	sthq->snr_estimate = snr_estimate;
	sthq->lock_value   = lock_value;

#ifdef DEBUG_PRINTF
	PRINTF("PUSH ms_count=%d, write_pointer=%d, ip=%f, qp = %f\n", 
			ms_count, sthq->write_pointer, ip, qp);
#endif
	releaseMutexForTrackHistoryQueue(sthq);
}

// return 0 if pop succeeded.
int  popFromTrackHistoryQueue(SatelliteTrackHistoryQueue* sthq,
					uint32_t *ms_count,
					float *f, float *p,
					int32_t *prompt_code_phase,
					float *ip, float *qp, float* snr_est, float* lock_val)
{
	int ret_val = 1;
	acquireMutexForTrackHistoryQueue(sthq);

	int n_entries = numberOfEntriesInTrackHistoryQueue(sthq);

	if(n_entries > 0)
	{
		uint32_t rp = sthq->read_pointer;
		ret_val = 0;
		*ms_count = sthq->ms_count[rp];
		*prompt_code_phase = sthq->prompt_code_phase[rp];
		*f = sthq->estimated_doppler[rp];
		*p = sthq->estimated_phase[rp];
		*ip = sthq->ip[rp];
		*qp = sthq->qp[rp];
		*snr_est = sthq->snr_estimate;
		*lock_val = sthq->lock_value;
		sthq->read_pointer = (rp + 1) & IQ_SAMPLE_DEPTH_MASK;
#ifdef DEBUG_PRINTF
		PRINTF("POP ms_count=%d, read_pointer=%d, ip=%f, qp = %f\n", 
				*ms_count, sthq->read_pointer, *ip, *qp);
#endif
	}

	releaseMutexForTrackHistoryQueue(sthq);
	return(ret_val);
}
// return 1 if entry is valid
int getEntryInTrackHistoryQueue(SatelliteTrackHistoryQueue* sthq,
					int32_t offset_from_read_pointer,
					uint32_t *ms_count,
					float* f, float* p,
					int32_t *prompt_code_phase,
					float *ip, float *qp, float* snr_est, float* lock_val)
{
	int ret_val = 1;
	acquireMutexForTrackHistoryQueue(sthq);
	int n_entries = numberOfEntriesInTrackHistoryQueue(sthq);
	if(n_entries >= (offset_from_read_pointer + 1))
	{
		ret_val = 0;
		int ri = (sthq->read_pointer + offset_from_read_pointer) & IQ_SAMPLE_DEPTH_MASK;
		*ms_count = sthq->ms_count[ri]; 
		*prompt_code_phase = sthq->prompt_code_phase[ri];
		*f = sthq->estimated_doppler[ri];
		*p = sthq->estimated_phase[ri];
		*ip = sthq->ip[ri];
		*qp = sthq->qp[ri];
		*snr_est = sthq->snr_estimate;
		*lock_val = sthq->lock_value;
	}
	releaseMutexForTrackHistoryQueue(sthq);
	return(ret_val);

}

int numberOfEntriesInTrackHistoryQueue(SatelliteTrackHistoryQueue* sthq)
{
	int ret_val = 0;
	if(sthq->write_pointer > sthq->read_pointer)
	{
		ret_val = (sthq->write_pointer - sthq->read_pointer);
	}
	else if(sthq->read_pointer > sthq->write_pointer)
	{
		ret_val = (sthq->write_pointer + IQ_SAMPLE_DEPTH) - sthq->read_pointer;
	}
	return(ret_val);
}

// lock check on track qp results.
int lockCheckOnTrackHistoryQueue(SatelliteTrackHistoryQueue* sthq)
{
	// TODO.
	return(1);
}
