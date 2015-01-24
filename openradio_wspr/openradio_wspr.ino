/*
  OpenRadio WSPR Transmitter
  Main.

  This uses the current (as of 2015-01-24) 'jason' branch of:
  https://github.com/etherkit/Si5351Arduino.git

  In this branch, the set_freq command accepts frequencies in units of one hundredth of a Hz (0.01Hz). I.e. 14.070MHz = 1407000000

  This code doesn't (currently) calculate the WSPR symbols internally. Instead you need
  to hard-code the symbol table.
  
  Copyright (C) 2014 Mark Jessop <vk5qi@rfhead.net>

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
#include <Wire.h>
#include "si5351.h"
#include "pins.h"
#include "TimerOne.h"

#define SERIAL_BAUD     57600


//
// USER SETTINGS
//

#define BASE_FREQ 1409715500UL // WSPR Transmission Frequency, in Hz*100.
float cal_factor = 0.99997100; // Calibration factor.

// WSPR Symbol Sequence.
// Replace this with the output from WSPR.exe (or equivalent)

uint8_t wspr[162] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };



// 4-FSK WSPR tones, specified as offsets from the base freqency in 0.01Hz increments.
uint32_t wspr_tones[4] = {0, 146, 293, 440}; 

Si5351 si5351; // Singleton instance of the Si5351 driver.

void setup(){
	// Initialise IO
	Serial.begin(SERIAL_BAUD);
    pinMode(TX_RX_SWITCH, OUTPUT);
    digitalWrite(TX_RX_SWITCH, LOW);
    pinMode(MODULATION, OUTPUT);
    digitalWrite(MODULATION, LOW);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    // Init Si5351.
    si5351.init(SI5351_CRYSTAL_LOAD_8PF);
    si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);

    // Setup frequencies, and disable all clocks.
    set_tx_freq(BASE_FREQ);
    si5351.clock_enable(RX_CLOCK,0);
    si5351.clock_enable(TX_CLOCK,0);
    si5351.clock_enable(SPARE_CLOCK,0);

    Serial.println("Ready.");

}


void loop(){
  if(Serial.available()){
    // Read in a byte.
    char inChar = (char)Serial.read();


    if(inChar == 't'){ // Transmit a WSPR sequence.
      transmit_wspr();
    }else if(inChar == 'a'){ // Testing commands, for checking the tone frequencies.
      si5351.clock_enable(TX_CLOCK,1);
      set_tx_freq(BASE_FREQ + wspr_tones[0]);
    }else if(inChar == 's'){
      si5351.clock_enable(TX_CLOCK,1);
      set_tx_freq(BASE_FREQ + wspr_tones[1]);
    }else if(inChar == 'd'){
      si5351.clock_enable(TX_CLOCK,1);
      set_tx_freq(BASE_FREQ + wspr_tones[2]);
    }else if(inChar == 'f'){
      si5351.clock_enable(TX_CLOCK,1);
      set_tx_freq(BASE_FREQ + wspr_tones[3]);
    }else if(inChar == 'g'){ 
      si5351.clock_enable(TX_CLOCK,0);
    }else{
     // Discard
    }
  }

}

// Wrapper function to handle including calibration factor.
static void set_tx_freq(uint32_t freq)
{
    // Let driver choose PLL settings. May glitch when changing frequencies.
    si5351.set_freq(freq * cal_factor, 0, TX_CLOCK);
}


// WSPR Transmission function.
//
// Step through each symbol in the wspr symbol array, and reprogram the Si5351 to the
// appropriate frequency. 
// Since all interrupts are disabled when a timer ISR is called, we have to do the I2C
// work in the main thread, on command from the ISR.

uint8_t tone_ptr = 0; // Pointer to the current symbol
volatile uint8_t next_tone = 0; // Incremented by the ISR
uint8_t next_tone2 = 0; // Local store, to compare against.
uint8_t step_tone = 0; // Flag used to signify we need to move to the next symbol.

void transmit_wspr(){
  // Start Transmitter
  digitalWrite(TX_RX_SWITCH, HIGH);

  // Start the timer interrupt, is called every 8192/12000 seconds.
  Timer1.initialize(682666UL);
  Timer1.attachInterrupt(wspr_isr);

  // Transmit!
  while(1){

    // We need to do the increment checking without the interrupts running, in case it gets
    // modified while we are checking.
    noInterrupts();
    if(next_tone>next_tone2){
      step_tone = 1;  // next_tone has incremented, raise flag
      next_tone2 = next_tone;
    }
    interrupts();

    if(step_tone){// Got a call from the ISR to increment the tone
      step_tone = 0;

      // We enable the oscillator here to avoid having it be active before the first tone
      // is due to be transmitted.
      si5351.clock_enable(TX_CLOCK,1);

      // Set the transmit frequency. This takes just <1ms to run.
      set_tx_freq(BASE_FREQ + wspr_tones[wspr[tone_ptr++]]);

      // If we're at the end of the symbol array, disable the timer and break.
      if(tone_ptr==162){
        Timer1.detachInterrupt();
        next_tone = 0;
        next_tone2 = 0;
        tone_ptr = 0;
        break;
      }
    }

  }

  // Disable transmitter.
  si5351.clock_enable(TX_CLOCK,0);
  digitalWrite(TX_RX_SWITCH, LOW);
  Serial.println("OK");

}

// Timer ISR.
void wspr_isr(void){
  next_tone += 1;
}


