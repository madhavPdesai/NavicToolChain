from __future__ import division, print_function

import numpy as np
from scipy import signal
import matplotlib.pyplot as plt

def plot_response(fs, w, h, title):
    plt.figure()
    plt.plot(0.5*fs*w/np.pi, 20*np.log10(np.abs(h)))
    plt.ylim(-40, 100)
    plt.xlim(0, 0.5*fs)
    plt.grid(True)
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Gain (dB)')
    plt.title(title)


filename="lpf.txt"
taps = np.loadtxt(filename,delimiter=",")
print (taps)

fs=560000000.0

w, h = signal.freqz(taps, [1], worN=2000)
plot_response(fs, w, h, "Low-pass Filter")

plt.show()
