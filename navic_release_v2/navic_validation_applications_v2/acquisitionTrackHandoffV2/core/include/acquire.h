#ifndef _acquire_h___
#define _acquire_h___
#include <stdint.h>
#include <navic_includes.h>
#include <satellite.h>

void scheduleCoarseAcquireSatellite (CoprocessorState* cp_state, SatelliteState* ss);
void scheduleFineAcquireSatellite (CoprocessorState* cp_state, SatelliteState* ss);
float estimateAcquisitionThreshold(CoprocessorState* cp_state, int8_t rf_band);

#endif
