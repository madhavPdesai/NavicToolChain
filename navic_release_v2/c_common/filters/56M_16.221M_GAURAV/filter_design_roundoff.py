# !python
# borrowed from https://gist.github.com/WarrenWeckesser/67bef61f496080aeb798

from __future__ import division, print_function

import numpy as np
from scipy import signal
import matplotlib.pyplot as plt

# round off to 8-bit int.
def round_off(taps):
    rounded_taps = np.zeros (len(taps), dtype=int)
    for I in range(len(taps)):
        rounded_taps[I] = (round (taps[I] * 128.0))
        print (I, taps[I], rounded_taps[I])
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


# Low-pass filter design parameters
fs = 56000000.0       # Sample rate, Hz
cutoff = 1300000.0    # Desired cutoff frequency, Hz
trans_width = 13000.0  # Width of transition from pass band to stop band, Hz
numtaps = 128      # Size of the FIR filter.
#fs = 56000000.0       # Sample rate, Hz
#cutoff = 1300000.0          # Desired cutoff frequency, Hz
#trans_width = 13000         # Width of transition from pass band to stop band, Hz
#numtaps = 128               # Size of the FIR filter.

taps = signal.remez(numtaps, [0, cutoff, cutoff + trans_width, 0.5*fs],
                    [1, 0], Hz=fs)
rounded_taps = round_off(taps)
print (taps)
print (rounded_taps)
w, h = signal.freqz(rounded_taps, [1], worN=2000)
plot_response(fs, w, h, 100,  "Low-pass Filter")

# High-pass filter design parameters
fs = 22050.0       # Sample rate, Hz
cutoff = 2000.0    # Desired cutoff frequency, Hz
trans_width = 250  # Width of transition from pass band to stop band, Hz
numtaps = 125      # Size of the FIR filter.

taps = signal.remez(numtaps, [0, cutoff - trans_width, cutoff, 0.5*fs],
                    [0, 1], Hz=fs)
w, h = signal.freqz(taps, [1], worN=2000)

plot_response(fs, w, h, 5, "High-pass Filter")

# Band-pass filter design parameters
fs = 22050.0         # Sample rate, Hz
band = [2000, 5000]  # Desired pass band, Hz
trans_width = 260    # Width of transition from pass band to stop band, Hz
numtaps = 125        # Size of the FIR filter.

edges = [0, band[0] - trans_width,
         band[0], band[1],
         band[1] + trans_width, 0.5*fs]
taps = signal.remez(numtaps, edges, [0, 1, 0], Hz=fs)
w, h = signal.freqz(taps, [1], worN=2000)

plot_response(fs, w, h, 5, "Band-pass Filter")

# Band-stop filter design parameters
fs = 22050.0         # Sample rate, Hz
band = [6000, 8000]  # Desired stop band, Hz
trans_width = 200    # Width of transition from pass band to stop band, Hz
numtaps = 175        # Size of the FIR filter.

edges = [0, band[0] - trans_width,
         band[0], band[1],
         band[1] + trans_width, 0.5*fs]
taps = signal.remez(numtaps, edges, [1, 0, 1], Hz=fs)
w, h = signal.freqz(taps, [1], worN=2000)

plot_response(fs, w, h,5,  "Band-stop Filter")

plt.show()
