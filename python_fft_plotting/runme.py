
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

TIMEOUT_SECONDS = 1
PORT_BAUD_RATE = 38400
PORT = "COM6"
FFT_SIZE = 128

ser = serial.Serial(PORT, PORT_BAUD_RATE, timeout=TIMEOUT_SECONDS, writeTimeout=TIMEOUT_SECONDS)

with open("output.txt", "w") as o:
    o.write("Log")
    
# To handle keypress
pause = False
fig = plt.figure(figsize=(20, 6))

ax1 = fig.add_subplot(2,1,1)
ax2 = fig.add_subplot(2,1,2)
ax1.set_xlim(0,128)
ax1.set_ylim(-600,600)
ax2.set_xlim(0,128)
ax2.set_ylim(-600,600)
#ax1 = plt.axes(xlim=(0, 128), ylim=(-600, 600))
#ax2 = plt.axes(xlim=(0, 128), ylim=(-600, 600))

def onclick(event):
    global pause
    pause = not pause
    
fig.canvas.mpl_connect('button_press_event', onclick)

# update plot
def update(frameNum, a0, a1):
    if pause:
        return
    try:
        ser.write("S;")
        line = ""
        for i in range(0,FFT_SIZE):
            numToAdd = (ser.readline()).strip()
            if len(numToAdd) == 0:
                break
            line += numToAdd
            line += " "
        # print data
        data = [float(val) for val in line.split()]
        #with open("output.txt", "a") as o:
        #    for val in data:
        #        o.write("%f\n" % val)
        #    o.write("\n\n")
        if(len(data) == FFT_SIZE):
            print time.time()
            a0.set_data(range(FFT_SIZE),data)
            a1.set_data(range(FFT_SIZE),data)
        else:
            #raise Exception("Something went wrong")
            print "Something went wrong, continuing"
    except KeyboardInterrupt:
        print('exiting')

# Test serial port
ser.write("GET FFT_SIZE;")
print ser.readline()
        
print "Plotting data"
# set up animation
a0, = ax1.plot([], [], linestyle='--', marker='o')
a1, = ax2.plot([], [], linestyle='--', marker='o')
anim = animation.FuncAnimation(fig, update, 
							 fargs=(a0, a1), 
							 interval=1000)

# show plot
plt.show()

# clean up
#analogPlot.close()
ser.close()
print('exiting.')