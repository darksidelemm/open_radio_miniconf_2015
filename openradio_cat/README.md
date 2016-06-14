OpenRadio Computer Aided Transciever Interface
==============================================

This code provides an easy-to-use way of controlling the OpenRadio SDR from within hamlib compatible software. Most testing will be done via CubicSDR: https://github.com/cjcliffe/SoapySDR

Currently it aims to implement changing the RX frequency only, by emulating the protocol for an FT-895/897 radio. There may be a simpler way of doing this!

So far you can do:

rigctl -m 123 -r /dev/ttyUSB0 -s 9600 f

and it returns a number somehow related to the current frequency but incorrect. I think my BCD routines are not correct.

Doing

rigctl -m 123 -r /dev/ttyUSB0 -s 9600 F 7090000

sets it to something that may or may not be 7.090 MHz, 


TODO
====
[*] Get Started!
[*] Test on hardware
[ ] Make it work on hardware
[ ] implement TX somehow.


