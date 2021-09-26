% sampling rate
fs =  40.0e6                            % external sampling rate.

% Filter-1 cutoffs
a1 = 0;
a2 = +1.5e6;

% low-pass filter.
b = fir1(127,a2*2/fs);                  % 128 tap low pass filter.
B = 8;                                  % Number of bits
L = floor(log2((2^(B-1)-1)/max(b)));    % Round towards zero to avoid overflow
bsc = b*2^L;                            % scale 
lpf = round(bsc);                       % round: 

d=1

freqz(lpf,d,128,40000000)

fp=fopen('filters.h','a')
fprintf(fp, '\nint8_t FILTER_3_COEFFS[] = {')
fprintf(fp, '%d, ', lpf(1:127))
fprintf(fp, '%d}; ', lpf(128))
fclose(fp)
