/*
  OpenRadio CAT Interface 

  This is used to provide a 9600 bps serial interface which CAT control 
  software can talk to.

  Many pieces taken from the Arduino FT857d library by VE3BUX available
  at: http://ve3bux.com/?page_id=501
  
  Copyright (C) 2016 Paul Warren <pwarren@pwarren.id.au>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    For a full copy of the GNU General Public License, 
    see <http://www.gnu.org/licenses/>.

*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <avr/pgmspace.h>

#warning __PGMSPACE_H_

#include <EEPROM.h>
#include <Wire.h>

#include "si5351.h"
#include "TimerOne.h"

#include "settings.h"
#include "pins.h"
#include "ring_buffer.h"
#include "CAT.h"

#define VERSION "0.3"

// Default output frequency
#define RX_FREQ 7090000
#define TX_FREQ 7090000
#define FREQ_LIMIT_LOWER 100000
#define FREQ_LIMIT_UPPER 30000000

#define SERIAL_BAUD  9600

Si5351 si5351;

int tx_relay_state = 0;
int tx_state = 0;

static struct settings settings;

// Ring buffer for use with the interrupt-driven transmit buffer.
struct ring_buffer data_tx_buffer = { { 0 }, 0, 0};

/* Returns true if settings are valid */
static bool read_settings(void)
{
    uint8_t *p = (uint8_t *)&settings;
    int i = 0;

    while(i < sizeof(struct settings))
        *p++ = EEPROM.read(i++);

    return settings.magic == SETTINGS_MAGIC;
}

static void write_settings(void)
{
    uint8_t *p = (uint8_t *)&settings;
    int i = 0;

    // Invalidate checksum to guard against partial writes
    EEPROM.write(0, 0);

    while(i < sizeof(struct settings))
        EEPROM.write(i++, *p++);
}

// Input buffer for reading user input.
#define INPUTBUFLEN	64
//char inputBuffer[INPUTBUFLEN];
int inputBufferPtr = 0;
char tempBuffer[20]; // Buffer for string to int conversion.
String inputBuffer = "";
unsigned char * converted;

void setup(){
    // Initialise things.
    pinMode(TX_RX_SWITCH, OUTPUT);
    digitalWrite(TX_RX_SWITCH, LOW);
    pinMode(MODULATION, OUTPUT);
    digitalWrite(MODULATION, LOW);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    Serial.begin(SERIAL_BAUD);
    Serial.setTimeout(1000 * 10);

    uint8_t rev_id = si5351_init();

    // Click the relay as an 'audible' startup indicator.
    digitalWrite(TX_RX_SWITCH, HIGH);
    delay(300);
    digitalWrite(TX_RX_SWITCH, LOW);

    if (!read_settings()) {
        //Serial.println(F("Resetting settings to defaults"));
        settings.magic = SETTINGS_MAGIC;
        settings.rx_freq = RX_FREQ;
        settings.tx_freq = TX_FREQ;
        settings.cal_factor = 1.0f;
    }

    set_rx_freq(settings.rx_freq);
    set_tx_freq(settings.tx_freq);


}

void loop(){
  if(Serial.available()){
    char inChar = (char)Serial.read();
    
    //Serial.print("Got a Char: ");
    //Serial.println(inChar);
    inputBuffer += inChar;
    inputBufferPtr++;
    //Serial.print("inputBufferPtr: ");
    //Serial.println(inputBufferPtr);

    // CAT protocol comes in 5 bytes.
    // How to deal with misaligned reads?
    if( inputBufferPtr > 4 || (inputBufferPtr>INPUTBUFLEN)){
      //Serial.println("Got 5 chars, parsing!");
      parseCAT(inputBuffer);
      inputBuffer = "";
      inputBufferPtr = 0;
    }
  }
}

// CAT Parser function
int parseCAT(String input){
  /*
    All CAT commands to the radio should be received as 5-byte blocks.
    The commands are generally in the form of: 
    
    {P1,P2,P3,P4,CMD}
    where P1-P4 are parameters, usually in Binary Coded Decimal
    and CMD is the command code, for all codes see CAT.h
        
    This only implements frequency control and sends an acknowledgement 
    for any other command, without doing anything!
    
  */

  //Serial.println("Parsing CAT Command");
  switch(input[4]) {
  case CAT_FREQ_SET :
    //Serial.println("Setting Frequency");
    setFreq(input);
    break;
  case CAT_RX_FREQ_CMD :
    //Serial.println("Sending Frequency");
    sendFreq();
    break;
  default :
    //Serial.println("In Default");
    sendAck();
    break;
  }
  return 0;
}

int freqValid(uint32_t freq){
	return (freq>FREQ_LIMIT_LOWER) && (freq<FREQ_LIMIT_UPPER);
}

void sendAck(void) {
  Serial.write(0x00);
}

void sendErr(void) {
  Serial.write(0xF0);
}

void sendPacket(byte packet[5]) {
  for (int i=0; i<5; i++) {
    Serial.write(packet[i]);
  }
  Serial.flush();
  return;
}

void sendFreq(void) {
  unsigned char tempWord[4];
  byte rigFreq[5];
  rigFreq[4] = CAT_MODE_DIG;
  
  converted = to_bcd_be(tempWord, settings.rx_freq, 8);
  
  for (byte i=0; i<4; i++){
    rigFreq[i] = converted[i];
  }
  
  sendPacket(rigFreq);
}   

// Parse frequency, set SI5351 clock
int setFreq(String input) {
  /*
    ----Set Frequency------------------------------------------------
    {0x00,0x00,0x00,0x00,CAT_FREQ_SET}
    
    Tune the radio to a frequency as: {P1,P2,P3,P4,0x01}
    Parameters:
    P1-P4 = frequency as: aa,bb,cc,dd
    
    Eg: {0x01,0x40,0x07,0x00,0x01} tunes to 14.070 MHz
    Eg2:{0x14,0x43,0x90,0x00,0x01} tunes to 144.390MHz

    Eg3;{0x00,0x70,0x90,0x00,0x01} tunes to 7 090 000 Hz

  */

  unsigned long result = 0;
  byte rigFreq[4];

  for (int i=0; i < 4; i++) {
    rigFreq[i] = input[i];
  }
  
  result = from_bcd_be(rigFreq, 8);

  if(freqValid(result)){
    set_rx_freq(result);
    sendAck();
    return 0;
  }else{
    //Serial.println("Invalid input.");
    sendErr();
    return -1;
  }	
}


// BCD functions
// Taken from FT857D Libraries
unsigned long from_bcd_be(const  byte bcd_data[], unsigned bcd_len)
{
	int i;
	long f = 0;

	for (i=0; i < bcd_len/2; i++) {
		f *= 10;
		f += bcd_data[i]>>4;
		f *= 10;
		f += bcd_data[i] & 0x0f;
	}
	if (bcd_len&1) {
		f *= 10;
		f += bcd_data[bcd_len/2]>>4;
	}
	return f;
}

unsigned char * to_bcd_be( unsigned char bcd_data[], unsigned long  freq, unsigned bcd_len)
{
	int i;
	unsigned char a;

	if (bcd_len&1) {
		bcd_data[bcd_len/2] &= 0x0f;
		bcd_data[bcd_len/2] |= (freq%10)<<4;
/* NB: low nibble is left uncleared */
		freq /= 10;
	}
	for (i=(bcd_len/2)-1; i >= 0; i--) {
		a = freq%10;
		freq /= 10;
		a |= (freq%10)<<4;
		freq /= 10;
		bcd_data[i] = a;
	}
	return bcd_data;
}

// Si5351 Helper Functions
//

uint8_t si5351_init(){
  si5351.init(SI5351_CRYSTAL_LOAD_8PF);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  // Set output frequencies
  set_rx_freq(RX_FREQ);
  set_tx_freq(TX_FREQ);
  
  // Enable RX, and disable TX by default.
  si5351.clock_enable(RX_CLOCK,1);
  si5351.clock_enable(TX_CLOCK,0);
  si5351.clock_enable(SPARE_CLOCK,0);
  
  // Read the status register and return the chip revision ID.
  si5351.update_status();
  return si5351.dev_status.REVID;
  
}

static void set_rx_freq(uint32_t freq)
{
  // Save actual value
  settings.rx_freq = freq;
  
  // Quadrature mixer requires a 4X LO signal.
  si5351.set_freq(freq * settings.cal_factor * 4, SI5351_PLL_FIXED,
		  RX_CLOCK);
}

static void set_tx_freq(uint32_t freq)
{
  // Save actual value
  settings.tx_freq = freq;
  
  // Let driver choose PLL settings. May glitch when changing frequencies.
  si5351.set_freq(freq * settings.cal_factor, 0, TX_CLOCK);
}

void tx_enable(){
  si5351.clock_enable(TX_CLOCK,1);
  tx_state = 1;
}

void tx_disable(){
  si5351.clock_enable(TX_CLOCK,0);
  tx_state = 0;
}

void tx(){
  digitalWrite(TX_RX_SWITCH, HIGH);
  digitalWrite(LED, HIGH);
  tx_relay_state = 1;
}

void rx(){
  digitalWrite(TX_RX_SWITCH, LOW);
  digitalWrite(LED, LOW);
  tx_relay_state = 0;
}
