% sampling rate
fs =  56.0e6                            % external sampling rate.
fi =  3.9e6				% external if.

% Filter-1 cutoffs
a1 = fi - 1.3e6;
a2 = fi + 1.3e6

% band-pass filter.
b = fir1(127,[a1*2/fs, a2*2/fs]);       % 128 band pass filter.
B = 8;                                  % Number of bits
L = floor(log2((2^(B-1)-1)/max(b)));    % Round towards zero to avoid overflow
bsc = b*2^L;                            % scale 
lpf = round(bsc);                       % round: 

d = 1

freqz(lpf,d,128,56000000)

fp=fopen('filters.h','a')
fprintf(fp, '\nint8_t FILTER_1_COEFFS[128] = {')
fprintf(fp, '%d, ', lpf(1:127))
fprintf(fp, '%d}; ', lpf(128))
fclose(fp)
