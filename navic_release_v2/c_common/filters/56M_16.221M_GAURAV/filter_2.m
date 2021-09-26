% Center frequency
fi =  4.0e6;				% if
fs =  56.0e6				% external sampling rate.

% Filter-1 cutoffs
a1 = fi-1.3e6;
a2 = fi+1.3e6;

% BPF 2 
b = fir1(128,[a1*2/fs a2*2/fs]);	% 128 tap filter with pass band specified.
B = 8;     				% Number of bits
L = floor(log2((2^(B-1)-1)/max(b)));    % Round towards zero to avoid overflow
bsc = b*2^L;				% scale 
bpf2 = round(bsc);   			% round: 
