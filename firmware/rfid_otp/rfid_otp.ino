/*
    rfid-otp - An HMAC-OTP RFID token
    Copyright (C) 2013 Jon Kivinen 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <sha1.h>

#define COIL_PIN 3 //!< Pin used to drive the coil

#define PUBLIC_KEY "000"  //!< ID
#define PRIVATE_KEY ""    //!< Long key with high entropy

#define PUBLIC_KEY_ENCODED_LENGTH     3
#define ONE_TIME_TOKEN_ENCODED_LENGTH 7

#define BIT_DELAY 128 //!< delay between each transistion in microseconds (Manchester encoding)

byte device_info[8] = { 0 }; //!< device info (D00-07)
byte payload[32] = { 0 }; //!< data payload (D08-39)
byte row_sums[10] = { 0 }; //!< row parity bits (P0-9)
byte col_sums[4] = { 0 }; //!< column parity bits (PC0-3)

//!< Get count from EEPROM
unsigned long count() {
  unsigned long c = 0;
  for (int i = 0; i < sizeof(unsigned long); i++) {
    c |= EEPROM.read(i) << i;
  }
  return c;
}

//!< Save count to EEPROM
void count(unsigned long c) {  
  for (byte *p = (byte*) &c; p - (byte*)&c < sizeof(unsigned long); p++) {
    EEPROM.write(p - (byte*)&c, *p);
  }
}

void transmit_bit(byte data) {
  // -- Manchester Encoding -- 
  if (data & 0x01) {
    // Transmit 1 (high->low)
    digitalWrite(COIL_PIN, HIGH);
    delayMicroseconds(BIT_DELAY);
    digitalWrite(COIL_PIN, LOW);
    delayMicroseconds(BIT_DELAY);
  }
  else {
    // Transmit 0 (low->high)
    digitalWrite(COIL_PIN, LOW);
    delayMicroseconds(BIT_DELAY);
    digitalWrite(COIL_PIN, HIGH);
    delayMicroseconds(BIT_DELAY);
  }
}

void transmit_header() {
  transmit_bit(1);
  transmit_bit(1);
  transmit_bit(1);
  transmit_bit(1);
  transmit_bit(1);
  transmit_bit(1);
  transmit_bit(1);
  transmit_bit(1);
  transmit_bit(1);
}

void transmit_device_info() {
  // Send D00-03
  transmit_bit(device_info[0]);
  transmit_bit(device_info[1]);
  transmit_bit(device_info[2]);
  transmit_bit(device_info[3]);
  // Send P0
  transmit_bit(row_sums[0]);
  // Send D04-07
  transmit_bit(device_info[4]);
  transmit_bit(device_info[5]);
  transmit_bit(device_info[6]);
  transmit_bit(device_info[7]);
  // Send P1
  transmit_bit(row_sums[1]);
}

void transmit_footer() {
  transmit_bit(col_sums[0]);
  transmit_bit(col_sums[1]);
  transmit_bit(col_sums[2]);
  transmit_bit(col_sums[3]);
  transmit_bit(0);
}

void transmit_payload() {
  byte *p = payload;
  
  for(int row = 2; row < 12; row++) {
    // Send next nibble
    transmit_bit(*p++);
    transmit_bit(*p++);
    transmit_bit(*p++);
    transmit_bit(*p++);
    // Send checksum
    transmit_bit(row_sums[row]);
  }
}

void transmit_all() {
  transmit_header();
  transmit_device_info();
  transmit_payload();
  transmit_footer();
}

void calculate_row_sums() {
  byte offset = 0;
  for(int i = 0; i < 10; i++) {
    offset += 4;
    row_sums[i] = payload[offset] ^ payload[offset + 1] ^ payload[offset + 2] ^ payload[offset + 3];
  }
}

void calculate_col_sums() {
  for(int i = 0; i < 4; i++) {
    col_sums[i] = payload[i] ^ payload[i + 5] ^ payload[i + 10] ^ payload[i + 15]
                 ^ payload[i + 20] ^ payload[i + 25] ^ payload[i + 30] ^ payload[i + 35]
                 ^ payload[i + 40] ^ payload[i + 45];
  }
}

void setup() {
  unsigned long c = count();

  // Compute token
  Sha1.initHmac((const uint8_t*) PRIVATE_KEY, 32);
  Sha1.write(PUBLIC_KEY);
  Sha1.write(c);
  
  // Truncate token
  
  // Prepare payload
  
  // Update EEPROM
  count(++c);
}

void loop() {
  // Transmit data indefinitely
  transmit_all();
}
