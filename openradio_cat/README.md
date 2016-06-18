OpenRadio Computer Aided Transciever Interface
==============================================

This code provides a way of controlling the OpenRadio SDR from within hamlib compatible software. Most testing will be done via CubicSDR: https://github.com/cjcliffe/SoapySDR and rigctl.

Currently it implements changing the RX frequency only, by emulating the protocol for a Kenwood TS-480 radio. 

So far you can do:

rigctl -m 228 -r /dev/ttyUSB0 -s 9600 f

and it returns the current frequency

Doing

rigctl -m 228 -r /dev/ttyUSB0 -s 9600 F 7090000

sets it to 7.090MHz.

CubicSDR works fine via hamlib to set the centre frequency.


TODO
====
- [X] Get Started!
- [X] Test on hardware
- [X] Make it work on hardware
- [X] Test with CubicSDR
- [ ] implement TX somehow.


