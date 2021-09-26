% sampling rate
fs =  56.0e6				% external sampling rate.

% Filter-1 cutoffs
a1 = 0;
a2 = +1.3e6;

% low-pass filter.
b = fir1(128,a2*2/fs);	% 128 tap low pass filter.
B = 8;     				% Number of bits
L = floor(log2((2^(B-1)-1)/max(b)));    % Round towards zero to avoid overflow
bsc = b*2^L;				% scale 
lpf = round(bsc);   			% round: 
