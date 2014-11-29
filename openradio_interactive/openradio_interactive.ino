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
#include "TimerOne.h"


// Pin Definitions
#define TX_RX_SWITCH    2
#define MODULATION      3
#define LED             13

// Which Si5351 clock is used for what...
#define RX_CLOCK    SI5351_CLK0
#define TX_CLOCK    SI5351_CLK2
#define SPARE_CLOCK    SI5351_CLK1

// Default output frequency
#define RX_FREQ 27000000
#define TX_FREQ 27000000

#define UPPER_LIMIT 30000000
#define LOWER_LIMIT 100000

// Other settings
#define SERIAL_BAUD     57600

// Fast pin toggling.
#define digitalToggleFast(P) *portInputRegister(digitalPinToPort(P)) = digitalPinToBitMask(P)

Si5351 si5351;

int tx_relay_state = 0;
int tx_state = 0;
uint32_t rx_freq = RX_FREQ;
uint32_t tx_freq = TX_FREQ;

// Ring buffer for use with the interrupt-driven transmit buffer.
#define TX_BUFFER_SIZE  64
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
    Serial.print(F("Starting up Si5351... "));
    uint8_t rev_id = si5351_init();
    Serial.print(F("Got Rev ID "));
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
    Serial.println(F("1: View current settings."));
    Serial.println(F("2: Change RX Frequency."));
    Serial.println(F("3: Change TX Frequency."));
    Serial.println(F("4: Toggle TX/RX Relay."));
    Serial.println(F("5: Toggle TX State."));
    Serial.println(F("6: Start BPSK31 Terminal"));
    Serial.println(F("7: RX VFO Mode"));
    Serial.println(F("8: Calibration Mode"));
    
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
            pskTerminal(31);
            break;
        case '7':
            rx_vfo();
            break;
        case '8':
            calibrate();
            break;
        default:
            break;
    }

}


//
// Menu Helper Functions
//

// Prompt user for a RX frequency.
void read_rx_freq(){
    while(Serial.available()>0){ Serial.read();} // Flush the input buffer
    Serial.print(F("Enter Frequency in kHz (XXXXX): "));
    while(Serial.available()<7){}
    int temp_freq = Serial.parseInt();
    if(temp_freq<30000 && temp_freq>100){
        uint32_t temp_freq2 = (uint32_t)temp_freq * 1000L;
        set_rx_freq(temp_freq2);
            Serial.println("");
    Serial.print(F("Frequency set to "));
    Serial.println(rx_freq);
    }else{
        Serial.println(F("Invalid frequency."));
    }

}

// Prompt user for a TX frequency.
void read_tx_freq(){
    while(Serial.available()>0){ Serial.read();} // Flush the input buffer
    Serial.print(F("Enter Frequency in kHz (XXXXX): "));
    while(Serial.available()<7){}
    int temp_freq = Serial.parseInt();
    if(temp_freq<30000 && temp_freq>100){
        uint32_t temp_freq2 = (uint32_t)temp_freq * 1000L;
        set_tx_freq(temp_freq2);
            Serial.println("");
    Serial.print(F("Frequency set to "));
    Serial.println(tx_freq);
    }else{
        Serial.println(F("Invalid frequency."));
    }
}

// Interactive receive tuning mode.
void rx_vfo(){
    Serial.println(F("RX VFO Mode, press q to exit.\n"));

    Serial.println(F("    Up: r   t    y    u    i    o    p"));
    Serial.println(F("  Down: f   g    h    j    k    l    ;"));
    Serial.println(F("Amount: 1   10  100   1K   10K 100K  1M"));

    while(1){
        if(Serial.available()>0){
            char c = Serial.read();
            if(c=='q') break;

            uint32_t temp = rx_freq;
            if(c=='r'){         temp += 1;
            }else if(c=='f'){   temp -= 1;
            }else if(c=='t'){   temp += 10;
            }else if(c=='g'){   temp -= 10;
            }else if(c=='y'){   temp += 100;
            }else if(c=='h'){   temp -= 100;
            }else if(c=='u'){   temp += 1000;
            }else if(c=='j'){   temp -= 1000;
            }else if(c=='i'){   temp += 10000;
            }else if(c=='k'){   temp -= 10000;
            }else if(c=='o'){   temp += 100000;
            }else if(c=='l'){   temp -= 100000;
            }else if(c=='p'){   temp += 1000000;
            }else if(c==';'){   temp -= 1000000;
            }else{// Do nothing
            }

            if(temp< UPPER_LIMIT && temp > LOWER_LIMIT){
                set_rx_freq(temp);
            }else{
                Serial.print("Out of range.");
            }
            Serial.print("RX Center Freq:"); Serial.println(rx_freq);
        }
    }
}

// Interactive calibration tuning mode.
void calibrate(){
    Serial.println(F("Calibration Mode, press q to exit.\n"));
    Serial.println(F("    Up: r   t    y"));
    Serial.println(F("  Down: f   g    h "));
    Serial.println(F("Amount: 1   10  100"));

    int32_t cal_val = si5351.get_correction();
    Serial.print(F("Current Correction Value: ")); Serial.println(cal_val);

    while(1){
        if(Serial.available()>0){
            char c = Serial.read();
            if(c=='q') break;

            uint32_t temp = cal_val;
            if(c=='r'){         temp += 1;
            }else if(c=='f'){   temp -= 1;
            }else if(c=='t'){   temp += 10;
            }else if(c=='g'){   temp -= 10;
            }else if(c=='y'){   temp += 100;
            }else if(c=='h'){   temp -= 100;
            }else{// Do nothing
            }
            si5351.set_correction(temp);
            set_rx_freq(rx_freq);
            cal_val = si5351.get_correction();
            Serial.print("Correction:"); Serial.println(cal_val);
        }
    }
    Serial.println(F("Correction value saved to Si5351 EEPROM."));
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
    Serial.print(F("\nReceive Center Frequency (Hz): ")); Serial.println(rx_freq);
    Serial.print(F("Transmit Frequency (Hz): ")); Serial.println(tx_freq);
    Serial.print(F("TX/RX Relay State: "));
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
}

/*
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
*/
int pskTerminal(int baud_rate){
    char* endptr;
    uint8_t exit_count = 0;
    Serial.println(F("Starting BPSK31 Terminal. Press ` to exit."));
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
    while(data_waiting(&data_tx_buffer)>0){}
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
    // Let driver choose PLL settings. May glitch when changing frequencies.
    si5351.set_freq(freq, 0, TX_CLOCK);
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


