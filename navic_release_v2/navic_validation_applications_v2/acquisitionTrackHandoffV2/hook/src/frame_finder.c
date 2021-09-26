#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ajit_access_routines.h>
#include <core_portme.h>
#include <macros.h>
#include <frame_finder.h>

void initFrameFinder(FrameFinder* bds, uint8_t sat_id)
{
	bds->sat_id = sat_id;
	bds->collected_byte = 0;
	bds->internal_counter = 0;
	bds->fsm_state = LOOKING_FOR_PREAMBLE;
}

void recordBitInFrameFinder(FrameFinder* bds, uint32_t ms_index, uint8_t ch, uint8_t bit)
{
	uint8_t cb = (bds->collected_byte << 1) | bit;
	//PRINTF("In Frame finder sat %d ms=%d bit=%d cb=0x%x\n", bds->sat_id, ms_index, bit, cb);
	switch(bds->fsm_state)
	{
		case LOOKING_FOR_PREAMBLE:
			if(cb == 0x8b) 
			{
				PRINTF("FrameSAT:%d %d 0x%x [preamble]\n",
						bds->sat_id, ms_index-7,cb);
				bds->internal_counter == 8;
				bds->fsm_state = FRAME_IN_PROGRESS;
			}

			bds->collected_byte = cb;
			break;
		case FRAME_IN_PROGRESS:
			bds->internal_counter++;
			if((bds->internal_counter & 0x7) == 0)
			{
				PRINTF("FrameSAT:%d 0x%x\n",
						bds->sat_id,cb);
			}
			if(bds->internal_counter == 300)
			{
				bds->fsm_state = LOOKING_FOR_PREAMBLE;
				PRINTF("FrameSAT:%d 0x%x\n",
						bds->sat_id,(cb << 4));
				bds->collected_byte = 0;
				bds->internal_counter = 0;
			}
			else
			{
				bds->collected_byte = cb;
			}
			break;
		default:
			break;
	}
}

