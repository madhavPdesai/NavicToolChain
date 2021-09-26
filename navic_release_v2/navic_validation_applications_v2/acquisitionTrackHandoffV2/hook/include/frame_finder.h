#ifndef frame_finder_h____
#define frame_finder_h____

typedef enum FrameFinderFsmState__ {
	LOOKING_FOR_PREAMBLE,
	FRAME_IN_PROGRESS
} FrameFinderFsmState;

typedef struct FrameFinder__ {
	uint8_t    sat_id;
	uint8_t    collected_byte;
	uint16_t   internal_counter; 
	FrameFinderFsmState fsm_state;
} FrameFinder;
void initFrameFinder(FrameFinder* bds, uint8_t sat_id);
void recordBitInFrameFinder(FrameFinder* bds, uint32_t ms_index, uint8_t ch, uint8_t bit);


#endif
