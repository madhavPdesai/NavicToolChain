# !python
# borrowed from https://gist.github.com/WarrenWeckesser/67bef61f496080aeb798
from __future__ import division, print_function
from __future__ import print_function

import sys
import getopt
import numpy as np
from scipy import signal
import matplotlib.pyplot as plt

# round off to 8-bit int.
def round_off(taps):
    rounded_taps = np.zeros (len(taps), dtype=int)
    max_tap = 0
    for I in range(len(taps)):
        if abs(taps[I]) > max_tap:
            max_tap = abs(taps[I])

    for I in range(len(taps)):
        rounded_taps[I] = (round (taps[I] * 127.0/max_tap))
        #print (I, taps[I], rounded_taps[I], max_tap)
        #np.append(rounded_taps, taps[I]) # round (taps[I]*100.0))

    return rounded_taps

def plot_response(fs, w, h, ylim, title):
    plt.figure()
    plt.plot(0.5*fs*w/np.pi, 20*np.log10(np.abs(h)))
    plt.ylim(-40, ylim)
    plt.xlim(0, 0.5*fs)
    plt.grid(True)
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Gain (dB)')
    plt.title(title)

# Band-pass filter design parameters
def bandpass_filter_gen(fs, center_freq, band_width, trans_width, numtaps):
    band = [center_freq-(band_width/2), center_freq+(band_width/2)]  # Desired pass band, Hz
    edges = [0, band[0] - trans_width,
         band[0], band[1],
         band[1] + trans_width, 0.5*fs]
    taps = signal.remez(numtaps, edges, [0, 1, 0], Hz=fs)
    rounded_taps = round_off(taps)
    return rounded_taps


# low-pass
def lowpass_filter_gen(fs,cutoff,trans_width, numtaps):
    taps = signal.remez(numtaps, [0, cutoff, cutoff + trans_width, 0.5*fs],
                    [1, 0], Hz=fs)
    rounded_taps = round_off(taps)
    return rounded_taps

# Usage
# filter_design [-B or -L] -s [sampling-freq] -c [center-freq (for BPF)] [-x cutoff-freq (for LPF)] [-w band-width (for BPD)] [-t transition-width] [-n ntaps] 
def printUsage():
    print ("filter_design [-B or -L] -s sampling-freq -c [center-freq (for BPF)] [-x cutoff-freq (for LPF)] [-w band-width (for BPF)] [-t transition-width] -n ntaps")
    print ("  Note: to generate a header file entry, add options -o <header-file-name> -f <filter-name>")
    print ("  Note: to display plot of frequency response, use -d")
    return 1


def main():
    bpf = False
    lpf = False
    fs  = None
    fcenter = None
    fcutoff = None
    bw = None
    twidth = None
    ntaps = None
    filter_name=None
    output_file_name=None
    display_flag = False

    arg_list = sys.argv[1:]

    opts,args = getopt.getopt(arg_list,'o:f:hBLs:c:x:w:t:n:d')
    for option, optarg in opts:
       if option == '-h':
          printUsage()
          return 0
       elif option == '-d':
          display_flag = True
       elif option == '-B':
          bpf = True
       elif option == '-L':
          lpf = True
       elif option == '-s':
          fs = float(optarg)
       elif option == '-c':
          fcenter = float (optarg)
       elif option == '-x':
          fcutoff = float (optarg)
       elif option == '-w':
          bw = float (optarg)
       elif option == '-t':
          twidth = float (optarg)
       elif option == '-n':
          ntaps = int (optarg)
       elif option == '-o':
          output_file_name = optarg
       elif option == '-f':
          filter_name = optarg
       else:
          print("Error: unknown option " + option)
          return 1
   
    if (fs == None):
       print ("Error: sample rate not specified") 
       return 1
    if (ntaps == None):
       print ("Error: number of taps not specified") 
       return 1

    rounded_taps = None
    if bpf :
       if (fcenter == None) or (twidth == None):
          print ("Error: for BPF, you need to specify center-frequency, bandwidth and  transition width")
          return 1
       else:
          rounded_taps = bandpass_filter_gen (fs, fcenter, bw, twidth, ntaps)
    elif lpf:
       if (fcutoff == None) or (twidth == None):
          print ("Error: for BPF, you need to specify cutoff-frequency and transition width")
          return 1
       else:
          rounded_taps = lowpass_filter_gen(fs, fcutoff, twidth, ntaps)
    else:
       print ("Error: at least one of -B or -L must be specified")
       return 1
       
    print ("Final Taps")
    for I in range(len(rounded_taps)):
        print (rounded_taps[I])

    if (output_file_name != None):
       if (filter_name == None):
          print ("Error: if you specify -o outfile, then you should also specify -f filter-name")
          return 1
       else:
          ofile = open(output_file_name, 'a')
          ofile.write ("int8_t " + filter_name + "_COEFFS[] = {")
          for I in range(len(rounded_taps)):
              ofile.write (str(rounded_taps[I]))
              if (I < (ntaps-1)):
                 ofile.write (", ")
              else:
                 break
          ofile.write ("};\n")
          ofile.close()
            

    if display_flag:
       w, h = signal.freqz(rounded_taps, [1], worN=2000)
       plot_response(fs, w, h, 100, "Band-pass Filter")
       plt.show()

    return 0

if __name__ == '__main__':
    ret_val = main()
    sys.exit(ret_val)
