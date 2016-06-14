#!/usr/bin/env python

import serial
import time

port = "/dev/ttyUSB0"
ard = serial.Serial(port,9600,timeout=5)
ard.setDTR(False)

packet1 = bytearray([0x02,0x80,0x99,0x00,0x01])
packet2 = bytearray([0x00,0x00,0x00,0x00,0x03])

time.sleep(5)

print "Sending: packet1",
ard.write(packet1)

from_ard = ard.read(1)
if from_ard == 0x00:
    print "Yay!"
else:
    print "From arduino: ",
    print from_ard.encode('hex')

print "Sending: packet2",
ard.write(packet2)

from_ard = ard.read(5)

print "From arduino: ",
print from_ard.encode('hex')

ard.close()
