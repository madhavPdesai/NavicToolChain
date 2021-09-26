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
#include <satellite.h>
#include <utils.h>

extern SatelliteState  volatile satellite_status[ACTIVE_NUMBER_OF_SATELLITES];
float estimateAcquisitionThreshold(CoprocessorState* cp_state, int8_t rf_band)
{
	return(ACQUISITION_THRESHOLD);	
}


void scheduleCoarseAcquireSatellite (CoprocessorState* cp_state, SatelliteState* ss)
{
	int sat_id = ss->sat_id;
	int band   = ss->rf_band;

#ifdef DEBUG_PRINTF
	PRINTF("Info: activating satellite %d (band %d) (enabled=%d).\n", 
				ss->sat_id, 
				band,
				ss->enabled);
#endif

	// activate the satellite (generate and copy its PRN into the Coprocessor)
	generatePrnAndActivateSatellite(cp_state, ss->sat_id, ss->rf_band);

	// parameters from coprocessor
	ss->coarse_acquisition_code_phase_max = cp_state->acquire_samples_per_ms - 2;
	ss->coarse_acquisition_code_phase_min = 0;
	ss->coarse_acquisition_code_phase_step = 2;

	ss->coarse_acquisition_doppler_max = COARSE_MAX_DOPPLER;
	ss->coarse_acquisition_doppler_min = COARSE_MIN_DOPPLER;
	ss->coarse_acquisition_doppler_bin_size = COARSE_DOPPLER_BIN;

	// bin count.
	float cp_bins = 
		(((float) (ss->coarse_acquisition_code_phase_max - ss->coarse_acquisition_code_phase_min))* 0.5) 
				+ 1.0;
	float doppler_bins = ((COARSE_MAX_DOPPLER - COARSE_MIN_DOPPLER)/COARSE_DOPPLER_BIN)+1.0;
	ss->coarse_acquisition_bin_count = cp_bins*doppler_bins;


	PRINTF("Info: schedule satellite %d (band %d) coarse acquire doppl.min=%f, doppl.max=%f, doppler.step=%f.\n",ss->sat_id, ss->rf_band,
			(double) ss->coarse_acquisition_doppler_min, //  doppler-min	
			(double) ss->coarse_acquisition_doppler_max,  //  doppler-max
			(double) ss->coarse_acquisition_doppler_bin_size  //  doppler-bin-size
	);
	scheduleAcquireCommand(cp_state,
			sat_id-1, // satellite-id
			1, // diff combining
			ss->rf_band, // l1 band
			NMS, // number of rf blocks to use
			ss->coarse_acquisition_doppler_min, //  doppler-min	
			ss->coarse_acquisition_doppler_max,  //  doppler-max
			ss->coarse_acquisition_doppler_bin_size,  //  doppler-bin-size
			ss->coarse_acquisition_code_phase_min,	 //  code-delay min
			ss->coarse_acquisition_code_phase_max,	 //  code-delay max
			ss->coarse_acquisition_code_phase_step,	 //  code-delay bin-size
			0);	 //  drop-threshold.

}

void scheduleFineAcquireSatellite (CoprocessorState* cp_state, SatelliteState* ss)
{
	ss->fine_acquisition_code_phase_max =
		addAcquisitionCodePhases (ss->coarse_acquisition_code_phase, 
						FINE_CODE_SEARCH_WIDTH/2);
	ss->fine_acquisition_code_phase_min =
		addAcquisitionCodePhases (ss->coarse_acquisition_code_phase, 
						-FINE_CODE_SEARCH_WIDTH/2);
	ss->fine_acquisition_code_phase_step = 2;

	ss->fine_acquisition_doppler_min = 
			ss->coarse_acquisition_doppler - FINE_DOPPLER_SEARCH_WIDTH/2;
	if(ss->fine_acquisition_doppler_min < 0.0)
		ss->fine_acquisition_doppler_min =  0.0;

	ss->fine_acquisition_doppler_max = 
			ss->coarse_acquisition_doppler + FINE_DOPPLER_SEARCH_WIDTH/2;
	ss->fine_acquisition_doppler_bin_size =  FINE_DOPPLER_SEARCH_BIN;


#ifdef DEBUG_PRINTF
			
	PRINTF("Info: schedule satellite %d (band %d) fine acquire: doppler range=%f,%f cphase rang=%d, %d\n",
			ss->sat_id,ss->rf_band,
			ss->fine_acquisition_doppler_min, ss->fine_acquisition_doppler_max,
			ss->fine_acquisition_code_phase_min, ss->fine_acquisition_code_phase_max
		);
#endif

	scheduleAcquireCommand(cp_state,
			ss->sat_id-1, // satellite-id
			1, // diff combining
			ss->rf_band, // l1 band
			10, // number of rf blocks to use
			ss->fine_acquisition_doppler_min,   //  doppler-min	
			ss->fine_acquisition_doppler_max,   //  doppler-max
			ss->fine_acquisition_doppler_bin_size,  //  doppler-bin-size
			ss->fine_acquisition_code_phase_min,	 //  code-delay min
			ss->fine_acquisition_code_phase_max,	 //  code-delay max
			ss->fine_acquisition_code_phase_step, 	 //  code-delay bin-size
			0);	 //  drop-threshold.

}

