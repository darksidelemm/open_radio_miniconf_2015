Open Radio Miniconf at LCA 2015
========================

This repository will be used for development of hardware and software for the Open Radio miniconf SDR project.

There are three folders here
* openradio_interactive: The main sketch for the miniconf
* production_test and si5351_bpsk_test are testing code that may be of interest

Building the Sketch
===================

1. Download arduino 1.0.6 (1.0.5 is ok, 1.5.8 is ok)
 * Debian/Ubuntu: ```apt-get install arduino```
 * CentOS/Red Hat: ```yum install arduino```
 * other: get the arduino IDE from http://arduino.cc
2. Run the IDE from this directory and open the ```openradio_interactive/openradio_interactive.ino``` file from within the IDE.
3. From the Tools -> Board menu, select ```Arduino Nano w/ATmega 328```
4. Click Verify (The tick button underneath the menubar), and it should say something along the lines of:
```
Binary sketch size: 22,212 bytes (of a 32,256 byte maximum)
```
Note: the reported size will vary depending on which avr-gcc compiler version is in use.

Uploading the Sketch
====================

Configure your serial port by going to 'Tools -> Serial Port' and selecting your serial port, most likely /dev/ttyUSB0

Then hit Upload (The arrow button underneath the menubar).


Connecting to the Open Radio
============================

Once the radio is up and running with the sketch loaded, you can connect to the radio via the Arduino Serial Monitor. Click 'Tools -> Serial Monitor'. 
Set the baud rate to 57600, and set line endings to CR+LF.
You should be presented with a menu:

```
1: View current settings.
2: Change RX Frequency.
3: Change TX Frequency.
4: Toggle TX/RX Relay.
5: Toggle TX State.
6: Start BPSK31 Terminal
7: RX VFO Mode
8: Calibration Mode
9: Save Settings
A: Set channel (TX/RX frequency pair)
B: Start Beacon
```

Pick your option and then press enter.  Minicom or Screen could also be used instead of the arduino serial monitor.
