#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <nco8bit.h>
#include <cp_rf_parameters.h>
#include <cp_internals.h>
#include <macros.h>

int main(int argc, char* argv[])
{
	if(argc < 3)
	{
		fprintf(stderr,"Usage:  %s <sampling-frequency> <frequency>\n", argv[0]);
		return(0);
	}

	double sampling_frequency  = atof(argv[1]);   // G:freq to be generated
	double signal_frequency  = atof(argv[2]);   // G:freq to be generated

	fprintf(stderr, "Info: sampling frequency = %f, signal frequency = %f\n", sampling_frequency, signal_frequency);

	uint64_t  signal_dphase; 
	uint64_t  sampling_dphase;


	// delta-phase.  Each sample corresponds to this much phase
	//   stored as a fixed point number with a total width of 33.
	double one_nco_rotation = ONE_NCO_ROTATION_61BIT;

	signal_dphase   = (uint64_t) (one_nco_rotation/signal_frequency);
	sampling_dphase = (uint64_t) (one_nco_rotation/sampling_frequency);
	uint64_t ss_ratio_u64 = (uint64_t) (one_nco_rotation * signal_frequency/sampling_frequency);

	fprintf(stderr,"sampling_dphase=0x%llx, signal_dphase = 0x%llx, ss_ratio_u64=0x%llx\n", 
				sampling_dphase, 
				signal_dphase, 
				ss_ratio_u64);

	double rev_sampling_freq = ((double) one_nco_rotation)/((int64_t) sampling_dphase);
	double rev_signal_freq   = sampling_frequency*ss_ratio_u64/one_nco_rotation;

	fprintf(stderr,"rev_sampling_freq=%f, rev_signal_freq = %f\n", rev_sampling_freq, rev_signal_freq);

	return 0;	
}

