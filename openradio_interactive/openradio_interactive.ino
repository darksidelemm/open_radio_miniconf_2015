/*
  OpenRadio Interactive Control Software 
  Main
  
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

#include <string.h>
#include "si5351.h"
#include "Wire.h"
#include <TimerOne.h>


// Pin Definitions
#define TX_RX_SWITCH    2
#define MODULATION      3
#define LED             13

// Which Si5351 clock is used for what...
#define RX_CLOCK    SI5351_CLK0
#define TX_CLOCK    SI5351_CLK2
#define SPARE_CLOCK    SI5351_CLK1

// Default output frequency
#define RX_FREQ 7100000
#define TX_FREQ 27001500

// Other settings
#define SERIAL_BAUD     57600

// Fast pin toggling.
#define digitalToggleFast(P) *portInputRegister(digitalPinToPort(P)) = digitalPinToBitMask(P)

Si5351 si5351;

int tx_relay_state = 0;
int tx_state = 0;
uint32_t rx_freq = RX_FREQ;
uint32_t tx_freq = TX_FREQ;


char relaymessage[40] = "Nothing set";

// Ring buffer for use with the interrupt-driven transmit buffer.
#define TX_BUFFER_SIZE  128
struct ring_buffer
{
  unsigned char buffer[TX_BUFFER_SIZE];
  volatile unsigned int head;
  volatile unsigned int tail;
};

ring_buffer data_tx_buffer = { { 0 }, 0, 0};

void setup(){
    
    // Initialise things.
    pinMode(TX_RX_SWITCH, OUTPUT);
    digitalWrite(TX_RX_SWITCH, LOW);
    pinMode(MODULATION, OUTPUT);
    digitalWrite(MODULATION, LOW);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    Serial.begin(SERIAL_BAUD);
    Serial.print("Starting up Si5351... ");
    uint8_t rev_id = si5351_init();
    Serial.print("Got Rev ID ");
    Serial.println(rev_id);

    // Click the relay as an 'audible' startup indicator.
    tx_enable();
    delay(300);
    tx_disable();

    print_state();

}

void loop(){
    while(Serial.available()>0){ Serial.read();} // Flush the input buffer
    print_state();
    Serial.println("");
    Serial.println("MENU:");
    Serial.println("1: View current settings.");
    Serial.println("2: Change RX Frequency.");
    Serial.println("3: Change TX Frequency.");
    Serial.println("4: Toggle TX/RX Relay.");
    Serial.println("5: Toggle TX State.");
    Serial.println("6: Set Message.");
    Serial.println("7: Start BPSK31 Terminal");
    
    while(Serial.available()==0){} // Wait for input
    
    char cmd = Serial.read();
    delay(300);
    while(Serial.available()>0){ Serial.read();} // Flush the input buffer
    Serial.println("");
    switch(cmd){
        case '1':
            print_state();
            break;
            
        case '2':
            read_rx_freq();
            break;
        
        case '3':
            read_tx_freq();
            break;
            
        case '4':
            toggle_tx_relay();
            break;
            
        case '5':
            toggle_tx();
            break;
        case '6':
            read_message();
            break;
        case '7':
            pskTerminal(31);
            break;
        default:
            break;
    }

}


//
// Menu Helper Functions
//

void read_rx_freq(){
    // TODO  
}

void read_tx_freq(){
    //TODO
}

void toggle_tx_relay(){
    if(tx_relay_state){
        rx();
    }else{
        tx();
    }
}

void toggle_tx(){
    if(tx_state){
        tx_disable();
    }else{
        tx_enable();
    }
}

void print_state(){
    Serial.print("\nReceive Center Frequency (Hz): "); Serial.println(rx_freq);
    Serial.print("Transmit Frequency (Hz): "); Serial.println(tx_freq);
    Serial.print("TX/RX Relay State: ");
    if(tx_relay_state){
        Serial.println("TX");
    }else{
        Serial.println("RX");
    }

    Serial.print("Transmitter State: ");
    if(tx_state){
        Serial.println("ON");
    }else{
        Serial.println("OFF");
    }
    Serial.print("Message: "); Serial.println(relaymessage);
}

void read_message(){
    while(Serial.available()>0){ Serial.read();} // Flush the input buffer
    Serial.print("Enter message, then LF: ");
    int i = 0;
    while(1){
        while(Serial.available()==0){} // Wait for a character
        char temp = Serial.read();
        if(temp == '\n' || temp == '\r'){
            relaymessage[i] = 0;
            Serial.println("");
            Serial.print("New Message: ");
            Serial.println(relaymessage);
            return;
        }else{
            relaymessage[i] = temp;
        }
        i++;
    }
}

void tx_bpsk31(){
    tx();
    tx_enable();
    bpsk_start(31);
    delay(1000);
    bpsk_add_data(relaymessage);
    bpsk_add_data("TESTING VK5QI");
    while(data_waiting(&data_tx_buffer)>0);
    delay(1000);
    bpsk_stop();
    tx_disable();
    rx();
}

int pskTerminal(int baud_rate){
    char* endptr;
    uint8_t exit_count = 0;
    Serial.println("Starting BPSK31 Terminal. Press ` to exit.");
    tx();
    tx_enable();
    bpsk_start(baud_rate);
    
    while(1){
        if(Serial.available()>0){
            char c = Serial.read();
            if(c=='`') break;
            Serial.print(c);// Echo to terminal.
            store_char(c,&data_tx_buffer); // Add the character to the transmit ring buffer.
        }
    }
    delay(500);
    bpsk_stop();
    tx_disable();
    rx();
}

//
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

// Quadrature mixer requires a 4X LO signal.
void set_rx_freq(uint32_t freq){
  si5351.set_freq(freq*4, SI5351_PLL_FIXED, RX_CLOCK);
  rx_freq = freq;
}

void set_tx_freq(uint32_t freq){
  si5351.set_freq(freq, SI5351_PLL_FIXED, TX_CLOCK);
  tx_freq = freq;
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


