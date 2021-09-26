#include <stdint.h>
#include <math.h>
#include <navic_includes.h>
#include <utils.h>
#include <app_defines.h>


float getTrackIf(int8_t rf_band)
{
	float ret_val = 
		((rf_band == RF_L1_BAND) ? L1_RESAMPLER_TRACK_IF :
			(rf_band == RF_L5_BAND) ? L5_RESAMPLER_TRACK_IF : S_RESAMPLER_TRACK_IF);
	return(ret_val);
}

const float one_over_2pi = 1.0/(2.0*M_PI);
				
float origCarrierFrequency(int8_t rf_band)
{
	float original_carrier_freq =
		(rf_band == RF_L1_BAND ? 
			GPS_L1_CARRIER_FREQ_IN_MHZ :
				((rf_band == RF_L5_BAND) ? 
					IRNSS_L5_CARRIER_FREQ_IN_MHZ :
					IRNSS_S_CARRIER_FREQ_IN_MHZ)) * 1000000.0;

	return(original_carrier_freq);
}


float normalizeTo2Pi(float x)
{
	float y = ((x < 0) ? -x : x);
	int R = (y * one_over_2pi);
	y = y - (2.0*M_PI*R);
	y = ((x < 0) ? ((2.0*M_PI) - y) : y);
	return(y);
}

int32_t addAcquisitionCodePhases(int32_t cphase, int32_t increment)
{
	int32_t result = cphase + increment;
	if(result < 0)
	{
		result = result + ACQUISITION_SAMPLES_PER_CODE;
	}
	else if(result > ACQUISITION_SAMPLES_PER_CODE)
	{
		result = result - ACQUISITION_SAMPLES_PER_CODE;
	}
	return(result);
}

int32_t addTrackingCodePhases(int32_t cphase, int32_t increment)
{
	int32_t result = cphase + increment;
	if(result < 0)
	{
		result = result + TRACKING_SAMPLES_PER_CODE;
	}
	else if(result > TRACKING_SAMPLES_PER_CODE)
	{
		result = result - TRACKING_SAMPLES_PER_CODE;
	}
	return(result);
}

void getTrackResults (uint32_t *out_args, 
				float *ie, float *qe,
				float *ip, float *qp,
				float *il, float *ql
			)
{
	// I = Integral (s(t) X -sin(wt + phi))
	*ie = -((float) ((int32_t) out_args[1]));

	// Q = Integral (s(t) X cos(wt + phi))	
	*qe = (float) ((int32_t) out_args[0]);

	*ip = -((float) ((int32_t) out_args[3]));
	*qp = (float) ((int32_t) out_args[2]);

	*il = -((float) ((int32_t) out_args[5]));
	*ql = (float) ((int32_t) out_args[4]);
}

		
