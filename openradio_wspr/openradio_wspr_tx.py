#!/usr/bin/env python
#
# OpenRadio WSPR Transmit
# Mark Jessop <vk5qi@rfhead.net>
# Call with python openradio_wspr_tx.py <duty cycle>
# i.e. python openradio_wspr_tx.py 0.5
# will transmit every second cycle (every 4 minutes)

import serial, datetime, time, sys


minute_mod = int(2/float(sys.argv[1]))

serial_port = "/dev/ttyUSB0"
serial_rate = 57600

s = serial.Serial(serial_port, serial_rate, timeout=1)

print "Transmitting every %d minutes." % (minute_mod)

while(True):
	now = datetime.datetime.now()
	if(now.second==0 and now.minute%minute_mod==0):
		print "Starting Transmission..."
		s.write('t')
		time.sleep(10)
	time.sleep(1)
