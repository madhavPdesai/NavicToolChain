#ifndef _utils_h___
#define _utils_h___
float getTrackIf(int8_t rf_band);
float normalizeTo2Pi(float x);
int32_t addAcquisitionCodePhases(int32_t cphase, int32_t increment);
int32_t addTrackingCodePhases(int32_t cphase, int32_t increment);
void getTrackResults (uint32_t *out_args, 
				float *ie, float *qe,
				float *ip, float *qp,
				float *il, float *ql
			);

float origCarrierFrequency(int8_t rf_band);
#endif
