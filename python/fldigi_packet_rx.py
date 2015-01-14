#!/usr/bin/env python
#
# Very quick hack to read bytes from fldigi's ARQ port and print the
#
# 2015-01-13 Mark Jessop <vk5qi@rfhead.net>
#

import socket, xmlrpclib, thread, time

# fldigi location
ARQ_HOST = "localhost"
ARQ_PORT = 7322

# See this link for fldigi's xmlrpc interface
# http://www.w1hkj.com/FldigiHelp-3.21/html/xmlrpc_control_page.html
XMLRPC_ADDR = "http://localhost:7362"

# Use the name that appears in the far bottom left in fldigi
FLDIGI_MODE = "BPSK250" # "BPSK500", "BPSK31", etc
FLDIGI_CARRIER = 1500 # Default offset for the OpenRadio preset channels.

class FldigiRX(object):
	def __init__(self, arq_host, arq_port, xmlrpc_addr, callback):
		self.callback = callback
		self.flrpc = xmlrpclib.ServerProxy(xmlrpc_addr)
		self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.s.connect((arq_host, arq_port))
		# Set fldigi's modem choice.
		self.flrpc.modem.set_by_name(FLDIGI_MODE)
		self.flrpc.modem.set_carrier(FLDIGI_CARRIER)
		# Turn automatic frequency control on, so fldigi will 'pull it'
		self.flrpc.main.set_afc(True)

	def run(self):
		while(True):
			data = self.s.recv(1)
			self.callback(data)


# An example callback function, which just prints out whatever data it gets.
def simple_callback(data):
	print data,


# Instantiate object
fldigi = FldigiRX(ARQ_HOST, ARQ_PORT, XMLRPC_ADDR, simple_callback)
# Start thread
thread.start_new_thread(fldigi.run, ())
# Run forever
while(True):
	time.sleep(1)
