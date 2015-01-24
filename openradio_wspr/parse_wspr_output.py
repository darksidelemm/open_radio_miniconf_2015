#!/usr/bin/env python
#
# WSPR.exe output parser. 
# Reads a file output by the WSPR.exe command line program and
# produces a line of code suitable for pasting into the openradio_wspr.ino
# code.
# 
# Mark Jessop <vk5qi@rfhead.net>
#
import sys

filename = sys.argv[1]

f = open(filename, "rb")

print "uint8_t wspr[162] = {",

for line in f:
 print str(int(line[5:6])) + ",",
print "};"

f.close()
