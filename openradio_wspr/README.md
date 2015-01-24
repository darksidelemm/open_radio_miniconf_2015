OpenRadio WSPR Beacon
=====================
This is an first-pass implementation of an OpenRadio-based WSPR beacon.
The only thing OpenRadio board specific is the TX/RX relay, which this code just locks to TX mode. It should be usable with any Arduino + Si5351.

I'm using @NT7S's development ('jason') branch of Si5351Arduino, in which he has added the ability to tune in 0.01Hz steps. This doesn't allow us to get the WSPR tones spot-on, but it's close enough to be decodable. The si5351.cpp and si5351.h in this directory are straight from his branch as of 2015-01-24.


How to use
==========

1. Get the WSPR command line version from http://physics.princeton.edu/pulsar/K1JT/WSPR.EXE . Perhaps there's a Linux version out there, but that version will run in WINE fine. 
2. Run it with the following (replace VK5QI and PF95 with your callsign and maidenhead locator):
```
wine WSPR.exe 0 0.0015 0 VK5QI PF95 20 11 > mysymbols.txt
```
Refer to the full instructions at http://physics.princeton.edu/pulsar/K1JT/WSPR_Instructions.TXT for more information on the command line arguments.
3. Convert the outputted data into a C array using the parse_wspr_output.py python script:
```
$ python parse_wspr_output.py mysymbols.txt 
uint8_t wspr[162] = { 3, 1, 0, 0, 0, 0, 2, 0, 1, 2, 0, 2, 3, 1, 1, 0, 2, 0, 1, 2, 0, 3, 2, 3, 3, 3, 1, 0, 0, 0, 0, 2, 0, 2, 1, 0, 2, 1, 2, 3, 2, 0, 0, 2, 0, 0, 1, 2, 3, 3, 0, 2, 3, 1, 0, 1, 2, 0, 0, 3, 3, 2, 1, 0, 2, 2, 0, 3, 3, 2, 3, 0, 1, 0, 3, 0, 1, 0, 0, 3, 2, 0, 3, 2, 1, 1, 2, 0, 2, 1, 3, 2, 3, 2, 1, 0, 0, 0, 1, 2, 0, 0, 0, 2, 3, 0, 2, 3, 2, 2, 1, 3, 1, 0, 3, 1, 2, 0, 1, 3, 2, 3, 2, 0, 0, 1, 3, 1, 2, 2, 0, 0, 0, 3, 0, 1, 2, 2, 3, 3, 2, 0, 2, 0, 0, 0, 2, 1, 1, 0, 3, 0, 3, 3, 0, 2, 0, 1, 3, 0, 0, 2, };
```
4. Copy and paste this line into openradio_wspr.ino at line 47, replacing the 'dummy' array. 
5. Modify the transmit frequency and calibration factor in openradio_wspr.ino to suit your needs, then compile and upload the openradio_wspr.ino sketch to your Arduino or OpenRadio board. 
6. Check the transmit functionality works by opening a serial console to the Arduino at 57600 baud, and sending an 'a' character. This will transmit a carrier. Press 'g' to stop.
7. Edit the openradio_wspr_tx.py script, and set the correct serial port for your system.
8. Use the openradio_wspr_tx.py to transmit a WSPR sequence at a given duty cycle. i.e. to transmit at a 50% duty cycle, use:
``` 
python openradio_wspr_tx.py 0.5
```

TODO
====
* Generation of WSPR symbols on the Arduino
* Command line argument parsing on the Python scripts.
* Get rid of the TimerOne library from the sketch directory.