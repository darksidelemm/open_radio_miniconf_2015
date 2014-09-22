/*
  Multi-mode HF data beacon software for Arduino + Si5351
  
  Copyright (C) 2014 Mark Jessop <vk5qi@rfhead.nt>

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


#include "si5351.h"
#include "Wire.h"
#include <TimerOne.h>


//
// Pin Definitions
//
#define MOD_PIN 10 // This is the pin we're wiring into the XOR gate, to invert our phase.
#define LED_PIN 13

//
// BPSK Modulation Settings
//
#define BAUD_RATE 31


//
// Output Settings
//
#define CLOCK_NUM SI5351_CLK0 // Use CLK0 for our output.
#define FREQ 27000000 // Frequency in Hz


// Fast pin toggling.
#define digitalToggleFast(P) *portInputRegister(digitalPinToPort(P)) = digitalPinToBitMask(P)

Si5351 si5351;

#define MSG_LEN_LIMIT 128
char txmessage[MSG_LEN_LIMIT] = "$$$$$$ VK5QI TESTING 1234567890\n";


// Ring buffer for use with the interrupt-driven transmit buffer.
#define TX_BUFFER_SIZE  128
struct ring_buffer
{
  unsigned char buffer[TX_BUFFER_SIZE];
  volatile unsigned int head;
  volatile unsigned int tail;
};

ring_buffer data_tx_buffer = { { 0 }, 0, 0};

void setup()
{
  // Start serial and initialize the Si5351
  Serial.begin(57600);

  // Initialise IO lines
  pinMode(MOD_PIN, OUTPUT);
  digitalWrite(MOD_PIN, LOW);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Start up the Si5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  si5351.set_freq(FREQ, SI5351_PLL_FIXED, CLOCK_NUM);
  si5351.clock_enable(CLOCK_NUM,1);


  //
  // Uncomment this section for BPSK
  //
  bpsk_start(BAUD_RATE); // This immediately starts transmitting phase revrsals.

  //
  // Uncomment this section for Morse
  //
  //morse_set_wpm(20);
}

void loop()
{

  //
  //  Uncomment this section for BPSK Modulation
  //
  delay(2000);
  bpsk_add_data("TESTING 1234567890");
  while(data_waiting(&data_tx_buffer)>0);
  delay(2000);

  //
  //  Uncomment this section for Morse Transmission.
  //
  //morse_tx_string("DE VK5QI TEST ");
  //delay(2000);


}

// Helper functions, used by the modulation libraries
void tx_on(){
  si5351.clock_enable(CLOCK_NUM,1);
}
void tx_off(){
  si5351.clock_enable(CLOCK_NUM,0);
}



