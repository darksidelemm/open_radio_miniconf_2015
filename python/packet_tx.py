#!/usr/bin/env python
#
# Quisk line-by-line messaging 'client' for the OpenRadio
#
# 2015-01-14 Mark Jessop <vk5qi@rfhead.net>
#

import serial, time, sys


# SERIAL PORT SETTINGS
# Set this as appropriate for your OS.
openradio_serial_port = "/dev/ttyUSB1"
openradio_serial_rate = 57600

psk_baud_rate = 250

packet_header = "$$$$"
packet_footer = "<EOM>"

print "Connecting to OpenRadio..."
or_serial = serial.Serial(openradio_serial_port, openradio_serial_rate, timeout=1)


def get_parameter(string):
    or_serial.write(string+"\n")
    return get_argument()
        
def set_parameter(string,arg):
    or_serial.write(string+","+arg+"\n")
    if get_argument() == arg:
        return True
    else:
        return False
    
def get_argument():
    data1 = or_serial.readline()
    # Do a couple of quick checks to see if there is useful data here
    if len(data1) == 0:
       return -1
        
    # Maybe we didn't catch an OK line?
    if data1.startswith('OK'):
       data1 = or_serial.readline()
        
    # Check to see if we have a comma in the string. If not, there is no argument.
    if data1.find(',') == -1:
       return -1
    
    data1 = data1.split(',')[1].rstrip('\r\n')
    
    # Check for the OK string
    data2 = or_serial.readline()
    if data2.startswith('OK'):
       return data1

time.sleep(2)

version = str(get_parameter("VER"))

print "Got version: %s\n\n" % version

print "Enter Nickname: ",
nick = raw_input()

print "Enter Channel: ",
channel = int(raw_input())

if(channel>30 or channel <1):
    print "Invalid channel."
    sys.exit(1)

set_parameter("CHAN", str(channel))

while(True):
    print '>',
    message = raw_input()
    or_serial.write("PSK,%d,%s<%s>%s\n" % (psk_baud_rate, packet_header, nick, message))

    print "Sending...",
    while(not("OK" in or_serial.readline())):
        time.sleep(0.5)
    print "OK"



