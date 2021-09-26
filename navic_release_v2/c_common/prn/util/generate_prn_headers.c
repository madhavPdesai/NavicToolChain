#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <irnss_prn_gen.h>

int main (int argc, char* argv[])
{
	PrnGenState prn_gen_state;
	int BAND;
	int SAT;

	fprintf(stdout,"// satellites 1-14 for L5, satellites 15-28 for S\n");
	fprintf(stdout,"int8_t irnss_prn_codes[] = {");

	int COUNT = 0;
	for(BAND=1; BAND <= 2; BAND++)
	{
		for(SAT=1; SAT <= 14; SAT++)
		{
			uint16_t chips=0;
			initPrnGen(BAND,SAT,&prn_gen_state);
			int J;
			
			fprintf(stdout,"\n// satellite %d, band %s\n",
						SAT, ((BAND ==1) ? "L1" : "S"));
			for(J=0; J < 1023; J++)
			{
				int8_t prn_val = prnGenTick(&prn_gen_state);
				prn_val = ((prn_val == 0) ? -1 : 1);
				if(COUNT > 0)
					fprintf(stdout,",");

				fprintf(stdout,"%d", prn_val);
				COUNT++;
			}
		}
	}	
	fprintf(stdout,"};\n");
	return(0);
}

