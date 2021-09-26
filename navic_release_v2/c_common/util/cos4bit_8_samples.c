#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define TSIZE 16
#define WSIZE 3

int main(int argc, char* argv[])
{
	int I;
	for(I=0; I < TSIZE/2; I++)
	{
		double phase   = (2*M_PI*I/TSIZE);

		double cval = cos(phase);
		int8_t abs_cval_fixed_point = (int) (((1 << (WSIZE-1))-1)*cval);
		fprintf(stdout,"$constant Cos_4x16_%d : $int<%d> := %d\n", I, WSIZE, (int8_t) abs_cval_fixed_point);
	}

	for(I=0; I < TSIZE/2; I++)
	{
		double phase   = (2*M_PI*I/TSIZE);

		double sval = sin(phase);
		int8_t abs_sval_fixed_point = (int) (((1 << (WSIZE-1))-1)*sval);
		fprintf(stdout,"$constant Sine_4x16_%d : $int<%d> := %d\n", I, WSIZE, (int8_t) abs_sval_fixed_point);
	}

	return(0);
}

