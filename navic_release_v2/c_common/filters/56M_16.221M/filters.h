double FILTER_INCOMING_SAMPLING_FREQUENCY = 56000000.0;
double FILTER_INCOMING_IF 		  = 16221000.0;
double FILTER_OUTGOING_TRACK_IF           =  4000000.0;
double FILTER_OUTGOING_ACQUIRE_IF	  =        0.0;
int8_t FILTER_1_COEFFS[128] = {0, 0, 0, -1, 0, 1, -1, -1, 1, 0, -2, 0, 2, -1, -1, 2, 0, -2, 1, 1, 0, 0, -1, 0, 3, -2, -3, 6, 2, -9, 3, 9, -8, -6, 11, 0, -11, 5, 7, -6, -2, 3, 0, 3, -5, -8, 16, 4, -28, 11, 34, -33, -25, 56, 1, -68, 35, 59, -70, -29, 90, -15, -86, 58, 58, -86, -15, 90, -29, -70, 59, 35, -68, 1, 56, -25, -33, 34, 11, -28, 4, 16, -8, -5, 3, 0, 3, -2, -6, 7, 5, -11, 0, 11, -6, -8, 9, 3, -9, 2, 6, -3, -2, 3, 0, -1, 0, 0, 1, 1, -2, 0, 2, -1, -1, 2, 0, -2, 0, 1, -1, -1, 1, 0, -1, 0, 0, 0}; 
int8_t FILTER_2_COEFFS[] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, -1, -2, -2, -2, -1, -1, 0, 0, 0, 0, -1, -2, -1, 0, 2, 5, 7, 8, 7, 5, 0, -5, -9, -12, -11, -8, -4, 0, 2, 2, 0, -4, -6, -5, 0, 10, 22, 33, 39, 36, 23, 0, -28, -54, -73, -77, -65, -38, 0, 40, 74, 92, 92, 74, 40, 0, -38, -65, -77, -73, -54, -28, 0, 23, 36, 39, 33, 22, 10, 0, -5, -6, -4, 0, 2, 2, 0, -4, -8, -11, -12, -9, -5, 0, 5, 7, 8, 7, 5, 2, 0, -1, -2, -1, 0, 0, 0, 0, -1, -1, -2, -2, -2, -1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0}; 
int8_t FILTER_3_COEFFS[] = {-1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 3, 3, 2, 1, 0, -1, -3, -4, -6, -8, -9, -11, -13, -14, -15, -16, -16, -16, -15, -13, -11, -8, -4, 1, 6, 12, 19, 26, 33, 41, 49, 57, 65, 73, 80, 87, 93, 98, 103, 106, 108, 110, 110, 108, 106, 103, 98, 93, 87, 80, 73, 65, 57, 49, 41, 33, 26, 19, 12, 6, 1, -4, -8, -11, -13, -15, -16, -16, -16, -15, -14, -13, -11, -9, -8, -6, -4, -3, -1, 0, 1, 2, 3, 3, 4, 4, 4, 4, 3, 3, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1}; 
