#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <irnss_prn_gen.h>

int main (int argc, char* argv[])
{
	PrnGenState prn_gen_state;
	int BAND;
	int SAT;

	for(BAND=1; BAND <= 2; BAND++)
	{
		for(SAT=1; SAT <= 14; SAT++)
		{
			uint16_t chips=0;
			initPrnGen(BAND,SAT,&prn_gen_state);
			int J;
			for(J=0; J < 10; J++)
			{
				chips = (chips << 1) | prnGenTick(&prn_gen_state);
			}
			fprintf(stderr,"%s %d %04o\n", ((BAND==1) ? "L5" : "S"),

					SAT, chips);
		}
	}	

	return(0);
}

