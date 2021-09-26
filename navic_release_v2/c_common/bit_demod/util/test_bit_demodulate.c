#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "bit_demodulate.h"

	
BitDemodFsmState bdfs;
//
// reads a file with tracking history data for a satellite,
// demodulates and dumps bits.
//
int main(int argc, char* argv[])
{
	initBitDemodFsmState (&bdfs);
	while(!feof(stdin))
	{
		uint32_t ms_count, bms_count;
		uint32_t code_phase;
		uint32_t b_code_phase;
		uint8_t bval;

		double ip, qp, doppler, phase, snr, lockval;
		double b_doppler, doppler_average, average_code_phase;
		uint8_t zero_crossings;
		double qp_average;
		
		char sacr_string[16];
		int n = fscanf(stdin,"%s %d %lf %lf %d %lf %lf %lf %lf ",sacr_string,
						&ms_count, &doppler, &phase,
						&code_phase, &ip, &qp, &snr, &lockval);
		if (n < 9)
			break;

		int status = 
			injestQp(&bdfs, ms_count, qp, code_phase, doppler, &bval, &bms_count, 
					&b_code_phase,  &average_code_phase,
					&b_doppler, &doppler_average, &zero_crossings, &qp_average);
		if(status == 0)
		{
			fprintf(stdout,"%d %d %lf %lf %lf %d %lf %d\n", 
				bms_count, b_code_phase,  average_code_phase,
				b_doppler, doppler_average, zero_crossings, qp_average,  bval);
		}
	}
}


