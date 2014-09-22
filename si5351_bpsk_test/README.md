Si5351 Modulation Test Sketch
========================

This Arduino sketch performs some basic modulation using the Si5351 clock generator IC.

If you want to have a go, you can get a Si5351 breakout board from Adafruit here:
https://www.adafruit.com/product/2045

Hardware Requirements:
* Si5351 wired up to an Arduino, via I2C. Depending on the breakout board you use, you might need some form of level conversion.
* To do BPSK modulation, wire the output of one of the Si5351 clock outputs into one input of a XOR gate (i.e. one gate from a 74AC86). Connect the other input of the XOR gate to an Arduino GPIO. By toggling the GPIO, the software is able to invert the clock signal, giving an effective 180 degree phase shift, and hence (very unclean) BPSK.

Software Requirements:
* TimerOne Arduino Library (google)
* Si5351Arduino Library, available from https://github.com/etherkit/Si5351Arduino


