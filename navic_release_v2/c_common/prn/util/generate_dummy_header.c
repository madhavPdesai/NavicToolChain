#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

int main (int argc, char* argv[])
{
	fprintf(stdout,"// dummy prn header.. \n");
	fprintf(stdout,"int8_t dummy_prn_code[] = {");
	fprintf(stdout,"\n// 1,-1,1,-1,.. \n");
	int J;
	int8_t prn_val = 1;
	for(J=0; J < 1023; J++)
	{
		if(J > 0)
			fprintf(stdout,",");

		fprintf(stdout,"%d", prn_val);
		prn_val = -prn_val;
	}

	fprintf(stdout,"\n// 1,1.. \n");
	for(J=0; J < 1023; J++)
	{
		fprintf(stdout,", 1");
	}

	fprintf(stdout,"};\n");
	return(0);
}

