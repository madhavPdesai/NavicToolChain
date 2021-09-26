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
#include <irnss_prn_headers.h>
#include <gps_prn_headers.h>


int8_t* getPrnPointer(int8_t sat_id)
{
	int8_t* prn_pointer = NULL;
	if(sat_id <= 32)
	{
		prn_pointer = &(gps_prn_codes[(sat_id-1)*1023]);
	}
	else if(sat_id <= 46)
	{
		prn_pointer = &(irnss_prn_codes[(sat_id-33)*1023]);
	}
	else if(sat_id <= 60)
	{
		prn_pointer = &(irnss_prn_codes[((sat_id-47) + 14)*1023]);
	}
	else if(sat_id == 61)
	{
		prn_pointer = &(irnss_prn_codes[0]);
	}
	else if(sat_id == 62)
	{
		prn_pointer = &(gps_prn_codes[0]);
	}
	else if(sat_id == 63)
	{
		prn_pointer = &(gps_prn_codes[0]);
	}
	return(prn_pointer);
}

