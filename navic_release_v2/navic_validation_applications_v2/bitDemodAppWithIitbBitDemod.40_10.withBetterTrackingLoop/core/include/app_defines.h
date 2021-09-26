#ifndef _app_defines_h___
#define _app_defines_h___


// there are 64 satellites active. 
#define ACTIVE_NUMBER_OF_SATELLITES 	 	64	

// Which bands? Edit to change this.
#define GPS_L1   	1  // enable GPS L1
#define IRNSS_L5 	1  // enable irnss L5
#define IRNSS_S		1  // enable irnss S

//
// coarse acquisition resolution is half code chip.
#define ACQUISITION_SAMPLES_PER_HALF_CODE_CHIP  2
#define ACQUISITION_SAMPLES_PER_CODE (4*1023)

//
// search for +/- 32 over the nominal coarse code delay
#define FINE_CODE_SEARCH_WIDTH 			64
// search for +/- 250MHz over nominal coarse doppler
#define FINE_DOPPLER_SEARCH_WIDTH		500.0
// search bin is 50Hz.
#define FINE_DOPPLER_SEARCH_BIN			50.0

//
// 1/2 chip separation between E,P,L correlators during tracking.
#define TRACKING_CORRELATOR_SEPARATION	 	 8
#define TRACKING_SAMPLES_PER_HALF_CODE_CHIP	 8
#define TRACKING_SAMPLES_PER_CODE (16*1023)

// This is invariant.
#define RESAMPLER_OUTPUT_SAMPLING_FREQUENCY 16368000.0

// For coarse search, number of intervals to be used 
// for coherent integration.
#define NMS 2

// tracking loop integration interval.
#define TRACK_NMS 		1

// coarse search parameters
#define COARSE_DOPPLER_BIN  	500.0
#define COARSE_MAX_DOPPLER 	10000.0
#define COARSE_MIN_DOPPLER 	0.0

//
// loop filter parameters.  These work quite well.
//    
// frequency locked loop...
//  
#define DOFFSET  1.0

#define K1_F    0.055
#define K2_F    1.0

// PLL
#define K1_P    0.75
#define K2_P    1.0

// DLL
#define K1_C    0.9
#define K2_C    0.1


// L1 ADC and filter parameters
#define L1_UPSAMPLE_FACTOR 		1
#define L1_ADC_SAMPLING_FREQUENCY 	40000000.0
#define L1_ADC_IF 			10000000.0
#define L1_RESAMPLER_TRACK_IF 		4000000.0
#define L1_ADC_FALLING_EDGE		0

// L5 ADC and filter parameters
#define L5_UPSAMPLE_FACTOR 		1
#define L5_ADC_SAMPLING_FREQUENCY 	40000000.0
#define L5_ADC_IF 			10000000.0
#define L5_RESAMPLER_TRACK_IF 		4000000.0
#define L5_ADC_FALLING_EDGE		0

// S ADC and filter parameters
#define S_UPSAMPLE_FACTOR 		1
#define S_ADC_SAMPLING_FREQUENCY 	40000000.0
#define S_ADC_IF 			10000000.0
#define S_RESAMPLER_TRACK_IF 		4000000.0
#define S_ADC_FALLING_EDGE		0


//
// nominal, overridden by dummy satellite 
// acquire.
//
#define ACQUISITION_THRESHOLD 			40000000.0
#define ACQUISITION_THRESHOLD_SCALE_FACTOR  	1.5

//
// depth of track history table
#define IQ_SAMPLE_DEPTH			32 			// must be a power of 2
#define IQ_SAMPLE_DEPTH_MASK    	(IQ_SAMPLE_DEPTH-1)	// depth -1

#define LOCK_THRESHOLD		   	0.6  	// should be close to 1.0
#define FINE_LOCK_THRESHOLD		0.95	// once past this value, start fine tracking.
#define SNR_LOCK_THRESHOLD	   	4 	// 4X or 6dB
#define LOCK_COMPUTATION_INTERVAL  	20 	// ms, 1 bit-period.
#define MAX_OUT_OF_LOCK_ITERATIONS 	8

#define SQUARE(x) ((x)*(x))

#endif
