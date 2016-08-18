
"""

Author: Gaute Gamnes

"""

import sys, serial, argparse
import numpy as np
from time import sleep
from collections import deque
import time

import matplotlib.pyplot as plt 
import matplotlib.animation as animation

TIMEOUT_SECONDS = 5
PORT_BAUD_RATE = 38400
PORT = "COM88"
FFT_SIZE = 128

ser = serial.Serial(PORT, PORT_BAUD_RATE, timeout=TIMEOUT_SECONDS, writeTimeout=TIMEOUT_SECONDS)

# update plot
def update(frameNum, a0, a1):
    try:
        ser.write("GET MAGNITUDES;")
        line = ""
        for i in range(0,FFT_SIZE):
            line += (ser.readline()).strip()
            line += " "
        data = [float(val) for val in line.split()]
        # print data
        if(len(data) == FFT_SIZE):
            print time.time()
            a0.set_data(range(FFT_SIZE),data)
            #a1.set_data(range(FFT_SIZE),data)
        else:
            raise Exception("Something went wrong")
    except KeyboardInterrupt:
        print('exiting')

# Test serial port
ser.write("GET FFT_SIZE;")
print ser.readline()
        
print "Plotting data"
# set up animation
fig = plt.figure()
ax = plt.axes(xlim=(0, 128), ylim=(0, 1500))
a0, = ax.plot([], [])
a1, = ax.plot([], [])
anim = animation.FuncAnimation(fig, update, 
							 fargs=(a0, a1), 
							 interval=100)

# show plot
plt.show()

# clean up
#analogPlot.close()
ser.close()
print('exiting.')