Open Radio Miniconf at LCA 2015
========================

This repository will be used for development of hardware and software for the Open Radio miniconf SDR project.

To start, we'll be working on code for the transmit side of the radio, which uses a Si5351A clock generator as a signal generator.


Building the Sketch
===================

1. Download arduino 1.0.6
 * Debian/Ubuntu: ```apt-get install arduino```
 * CentOS/Red Hat: ```yum install arduino```
 * other: get the arduino IDE from http://arduino.cc
2. Run the IDE from this directory and open the ```openradio_interactive/openradio_interactive.ino``` file from within the IDE.
3. From the Tools -> Board menu, select ```Arduino Nano w/ATmega 328```
4. Click Verify (The tick button underneath the menubar), and it should say something along the lines of:
```
Binary sketch size: 14,598 bytes (of a 32,256 byte maximum)
```

Uploading the Sketch
====================

Configure your serial port by going to 'Tools -> Serial Port' and selecting your serial port, most likely /dev/ttyUSB0

Then hit Upload (The arrow button underneath the menubar).
