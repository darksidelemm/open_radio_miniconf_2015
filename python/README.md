Example Python Scripts
======================

These provide some examples of how the OpenRadio board and fldigi can be used together to send and receive data.

* packet_tx.py - Transmit lines of text as BPSK, using the OpenRadio board. The code promps you for nickname and channel number on start, and then lets you send lines of text.
* fldigi_rx.py - Connects to fldigi, configures it for the appropriate mode, then prints out all received characters.

TODO
====
* Combine these programs into a kind of IRC-like chat interface. Maybe use curses?
* Make the receive program look for the packet header/footer, and only print out the packet contents.
* Do packet framing 'better'
* Move serial port / baud rate / other settings to command line options.
