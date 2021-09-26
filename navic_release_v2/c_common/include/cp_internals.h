#ifndef cs_side_headers_h___
#define cs_side_headers_h___

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define CHIPS_PER_MS				1023
#define RF_OVERSAMPLING_FACTOR 			16
#define RF_BLOCK_QUEUE_SIZE 			8
#define INTERNAL_SAMPLING_FREQUENCY             16*CHIPS_PER_MS*1000
#define INTERNAL_INTERMEDIATE_FREQUENCY		(4*CHIPS_PER_MS*1000)

// bits
#define RESAMPLED_BLOCK_SIZE			(RF_OVERSAMPLING_FACTOR*1023)
#define RESAMPLED_BLOCK_SIZE_IN_DWORDS		(RF_OVERSAMPLING_FACTOR*1024/64)

#define NUMBER_OF_SATELLITES			64
#define MAX_ACQUIRE_MILLISECOND_COUNT		8

#define	ONE_NCO_ROTATION   (2.0*(1u<<31))

#endif
