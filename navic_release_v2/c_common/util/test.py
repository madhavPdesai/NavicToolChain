from __future__ import division
import numpy as np
import matplotlib.pyplot as plt


filename="nco8.txt"

axes = plt.gca()
axes.set_ylim(-128, 128)

adc_data = np.loadtxt(filename,delimiter="\n")
jdx = np.argsort(adc_data)
print (jdx)
#print (jdx, adc_data)
plt.plot (adc_data)

plt.show()

ps = np.abs(np.fft.fft(adc_data))**2

time_step = 1.0 / 56000000.0
freqs = np.fft.fftfreq(adc_data.size, time_step)
idx = np.argsort(freqs)

plt.plot(freqs[idx], ps[idx])
plt.show()
