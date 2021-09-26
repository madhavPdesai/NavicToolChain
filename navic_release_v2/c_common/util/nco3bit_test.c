#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <nco.h>
#include <cp_rf_parameters.h>
#include <cp_internals.h>
#include <macros.h>

int main(int argc, char* argv[])
{
	if(argc < 3)
	{
		fprintf(stderr,"Usage:  %s <sampling-frequency> <frequency> <phase>\n", argv[0]);
		fprintf(stderr,"   sampling-frequency, frequency and phase should be floats\n");
		return(0);
	}

	double sampling_frequency  = atof(argv[1]);   // G:freq to be generated
	double frequency  = atof(argv[2]);   // G:freq to be generated
	double init_phase = atof(argv[3]);   // G: initial phase offset in radians

	uint32_t  m_phase, m_dphase; 


	// delta-phase.  Each sample corresponds to this much phase
	//   stored as a fixed point number with a total width of 33.
	double one_nco_rotation = ONE_NCO_ROTATION;

	double tmp = (frequency/sampling_frequency);
	m_dphase = (uint32_t) (tmp*one_nco_rotation);
	fprintf(stderr,"m_dphase = %llx\n", m_dphase);

	// initial phase:  init_phase in radians..  scaled.
	m_phase = (uint32_t) (init_phase*ONE_NCO_ROTATION/(2.0*M_PI));
	fprintf(stderr,"initial m_phase = %llx\n", m_phase);

	NcoState nco_state;
	initNcoState (&nco_state, m_phase , m_dphase);

	int i;
	int NSAMPLES = (int) sampling_frequency;
	
	double max_err = 0.0;
	for(i=0; i < NSAMPLES; i++)
	{
		int8_t sval, cval;
		nextNco3Output(&nco_state, &sval, &cval);		
		fprintf(stdout,"sine=%d\n",sval);
		fprintf(stdout,"cos=%d\n",cval);

		double table_val = (((double) cval)/((1 << 2)-1));
		double libm_val  = cos((2*M_PI*i*frequency/sampling_frequency) + init_phase);

		double error = fabs(table_val - libm_val);
		fprintf(stderr,"[%d] %d (%f) ref=%f coserror=%f\n", i, cval, table_val, libm_val, error);
		if(error > max_err)
			max_err = error;
	}

	fprintf(stderr,"Max error is %f\n", max_err);
	return 0;	
}

