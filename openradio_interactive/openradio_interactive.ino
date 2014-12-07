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

#include <avr/pgmspace.h>

#warning __PGMSPACE_H_

#include <EEPROM.h>
#include <Wire.h>

#include "si5351.h"
#include "TimerOne.h"

#include "settings.h"
#include "pins.h"
#include "ring_buffer.h"

// Default output frequency
#define RX_FREQ 27000000
#define TX_FREQ 27000000

#define UPPER_LIMIT 30000000
#define LOWER_LIMIT 100000

// Other settings
#define SERIAL_BAUD     57600

// Prototypes presnt in PSK.ino. Forward delcare them so ino can build
void bpsk_start(int baud_rate);
void bpsk_stop(void);

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
    Serial.print(F("Starting up Si5351... "));
    uint8_t rev_id = si5351_init();
    Serial.print(F("Got Rev ID "));
    Serial.println(rev_id);

    // Click the relay as an 'audible' startup indicator.
    tx_enable();
    delay(300);
    tx_disable();

    if (!read_settings()) {
        Serial.println(F("Resetting settings to defaults"));
        settings.magic = SETTINGS_MAGIC;
        settings.rx_freq = RX_FREQ;
        settings.tx_freq = TX_FREQ;
        settings.calibration = 0;
    }

    print_state();

}

static void flush_input(void)
{
    while (Serial.available() > 0)
        Serial.read();
}


void loop()
{
    flush_input();

    print_state();
    Serial.println("");
    Serial.println(F("MENU:"));
    Serial.println(F("1: View current settings."));
    Serial.println(F("2: Change RX Frequency."));
    Serial.println(F("3: Change TX Frequency."));
    Serial.println(F("4: Toggle TX/RX Relay."));
    Serial.println(F("5: Toggle TX State."));
    Serial.println(F("6: Start BPSK31 Terminal"));
    Serial.println(F("7: RX VFO Mode"));
    Serial.println(F("8: Calibration Mode"));
    Serial.println(F("9: Save Settings"));
    Serial.println(F("A: Set channel (TX/RX frequency pair)"));

    while (Serial.available() == 0); // Wait for input

    char cmd = Serial.read();
    flush_input();
    Serial.println("");

    switch(cmd) {
    case '1':
        // Do nothing, state will print along with menu
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
        pskTerminal(250);
        break;
    case '7':
        rx_vfo();
        break;
    case '8':
        calibrate();
        break;
    case '9':
        save_settings();
        break;
    case 'A':
    case 'a':
        set_channel();
        break;

    default:
        break;
    }
}


//
// Menu Helper Functions
//

static int32_t get_freq(void)
{
    int32_t freq;

    flush_input();

    Serial.print(F("Enter Frequency in kHz: "));
    freq = Serial.parseInt();
    Serial.println(freq);

    flush_input();

    if (freq < 30000 && freq > 100)
        return freq * 1000;

    Serial.println(F("Error: timeout or frequency out of range"));

    return -1;
}

static void show_freq(int32_t freq)
{
    Serial.print(F("\r\nFrequency set to "));
    Serial.print(freq);
    Serial.println(F(" Hz"));
}

// Prompt user for a RX frequency.
static void read_rx_freq(void)
{
    int32_t freq = get_freq();

    if (freq <= 0) {
        Serial.println(F("Invalid frequency."));
        return;
    }

    set_rx_freq(freq);
    show_freq(settings.rx_freq);
}

// Prompt user for a TX frequency.
static void read_tx_freq(void)
{
    int32_t freq = get_freq();

    if (freq <= 0) {
        Serial.println(F("Invalid frequency."));
        return;
    }

    set_tx_freq(freq);
    show_freq(settings.tx_freq);
}

// Interactive receive tuning mode.
static void rx_vfo(void)
{
    Serial.println(F("RX VFO Mode, press q to exit.\r\n"));

    Serial.println(F("    Up: r   t    y    u    i    o    p"));
    Serial.println(F("  Down: f   g    h    j    k    l    ;"));
    Serial.println(F("Amount: 1   10  100   1K   10K 100K  1M"));

    while (1) {
        if (Serial.available() > 0) {
            char c = Serial.read();

            uint32_t temp = settings.rx_freq;
            switch (c) {
            case 'q':
                flush_input();
                return;

            case 'r': temp += 1; break;
            case 'f': temp -= 1; break;
            case 't': temp += 10; break;
            case 'g': temp -= 10; break;
            case 'y': temp += 100; break;
            case 'h': temp -= 100; break;
            case 'u': temp += 1000; break;
            case 'j': temp -= 1000; break;
            case 'i': temp += 10000; break;
            case 'k': temp -= 10000; break;
            case 'o': temp += 100000; break;
            case 'l': temp -= 100000; break;
            case 'p': temp += 1000000; break;
            case ';': temp -= 1000000; break;

            default:
                // Do nothing
                continue;
            }

            if (temp < UPPER_LIMIT && temp > LOWER_LIMIT) {
                set_rx_freq(temp);
            } else {
                Serial.print(F("Out of range. "));
            }

            Serial.print(F("RX Center Freq: ")); Serial.println(settings.rx_freq);
        }
    }
}

// Interactive calibration tuning mode.
static void calibrate(void)
{
    Serial.println(F("Calibration Mode, press q to exit.\n"));
    Serial.println(F("    Up: r   t    y"));
    Serial.println(F("  Down: f   g    h "));
    Serial.println(F("Amount: 1   10  100"));

    int32_t cal_val = si5351.get_correction();
    Serial.print(F("Current Correction Value: ")); Serial.println(cal_val);

    while(1) {
        if (Serial.available() > 0) {
            char c = Serial.read();

            int32_t delta;

            switch (c) {
            case 'q':
                flush_input();
                return;
            case 'r': delta = 1; break;
            case 'f': delta = -1; break;
            case 't': delta = 10; break;
            case 'g': delta = -10; break;
            case 'y': delta = 100; break;
            case 'h': delta = -100; break;
            default:
                      break;
            }

            si5351.set_correction(cal_val + delta);
            set_rx_freq(settings.rx_freq);
            cal_val = si5351.get_correction();
            Serial.print(F("Correction:")); Serial.println(cal_val);
        }
    }
    Serial.println(F("Correction value saved to Si5351 EEPROM."));
}

static void save_settings(void)
{
    Serial.print("Saving settings...");
    write_settings();
    Serial.println("Done");
}

#define MIN_CHANNEL 1
#define MAX_CHANNEL 30

// From https://docs.google.com/spreadsheets/d/1KP5XsAHPCD2FsUW5RoqCfCJYRt2o03q6RN2TroPgEKk/edit#gid=0
const prog_uint16_t channel_list[] = {8986, 8990, 8993, 8996, 9000,
    9003, 9006, 9010, 9013, 9016, 9020, 9023, 9026, 9030, 9033, 9036, 9040, 9043,
    9046, 9050, 9053, 9056, 9060, 9063, 9066, 9070, 9073, 9076, 9080, 9083};

static void set_channel(void)
{
    int32_t chan;

    flush_input();

    Serial.print(F("Enter channel number (1-30): "));
    chan = Serial.parseInt();
    Serial.println(chan);

    flush_input();

    if (chan < MIN_CHANNEL || chan > MAX_CHANNEL) {
        Serial.println(F("Error: timeout or channel out of range"));
        return;
    }

    uint32_t rx = (uint32_t)pgm_read_word_near(channel_list + chan-1) * 1000;
    uint32_t tx = rx * 3 + 1500;

    set_rx_freq(rx);
    set_tx_freq(tx);

    // Arduino sucks
    Serial.print(F("Channel ")); Serial.print(chan);
    Serial.print(F(" selected: ")); Serial.print(rx);
    Serial.print(F(" Hz RX, ")); Serial.print(tx);
    Serial.println(F(" Hz TX"));
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

static void print_state(void)
{
    Serial.print(F("\r\nRX Frequency (Hz): ")); Serial.println(settings.rx_freq);
    Serial.print(F("TX Frequency (Hz): ")); Serial.println(settings.tx_freq);

    Serial.print(F("TX/RX Relay State: "));
    if (tx_relay_state)
        Serial.println("TX");
    else
        Serial.println("RX");

    Serial.print(F("Transmitter State: "));
    if (tx_state)
        Serial.println("ON");
    else
        Serial.println("OFF");
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
    settings.rx_freq = freq;
}

void set_tx_freq(uint32_t freq){
    // Let driver choose PLL settings. May glitch when changing frequencies.
    si5351.set_freq(freq, 0, TX_CLOCK);
    settings.tx_freq = freq;
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


