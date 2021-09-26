#ifndef IRNSS_PRN_GEN_H__
#define IRNSS_PRN_GEN_H__
typedef struct PrnGenState__ {
	// G1[1:10] is first shift register.
	int G1[11];

	// G2[1:10] is the second shift register.
	int G2[11];	

	int band;
	int sat_id;
} PrnGenState;


// band =1 means L5, band=2 means S.
// sat id must be at most 14.
void initPrnGen (int band, int sat_id, PrnGenState* prn_gen_state);

// generate prn bit and move forward.
int  prnGenTick  (PrnGenState* prn_gen_state);



/* Expected chips 
 id G1 L5     chips  G1 S     chips
 1 1110100111 0130 0011101111 1420
 2 0000100110 1731 0101111101 1202
 3 1000110100 0713 1000110001 0716
 4 0101110010 1215 0010101011 1524
 5 1110110000 0117 1010010001 0556
 6 0001101011 1624 0100101100 1323
 7 0000010100 1753 0010001110 1561
 8 0100110000 1317 0100100110 1331
 9 0010011000 1547 1100001110 0361
10 1101100100 0233 1010111110 0501
11 0001001100 1663 1110010001 0156
12 1101111100 0203 1101101001 0226
13 1011010010 0455 0101000101 1272
14 0111101010 1025 0100001101 1362
*/


#endif
