% Center frequency
fi =  16.221e6;
fs =  56.0e6

% Filter-1 cutoffs
a1 = fi-1.3e6;
a2 = fi+1.3e6;

% BPF 1 
b = fir1(128,[a1*2/fs a2*2/fs]);	% 128 tap filter with pass band specified.
B = 8;     				% Number of bits
L = floor(log2((2^(B-1)-1)/max(b)));    % Round towards zero to avoid overflow
bsc = b*2^L;				% scale 
bpf1 = round(bsc);   			% round: 
