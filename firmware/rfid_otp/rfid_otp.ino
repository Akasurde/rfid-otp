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

#include "config.h"

// Customizations for security (Defaults OK)
#define PAYLOAD_NIBBLES_ID  3
#define PAYLOAD_NIBBLES_OTP 7
//

#define COIL_PIN 3 //!< Pin used to drive the coil
#define BIT_DELAY 128 //!< delay between each transistion in microseconds (Manchester encoding)

byte payload[40] = { 0 }; //!< data payload (D00-39) where D00-D03 is /not/ considered special
byte row_sums[10] = { 0 }; //!< row parity bits (P0-9)
byte col_sums[4] = { 0 }; //!< column parity bits (PC0-3)

// Pointers into payload
byte *payload_id = payload;
byte *payload_otp = payload + 4*PAYLOAD_NIBBLES_ID;

char nibble_to_hex(byte nibble) {
  nibble &= 0x0F;
  return (nibble < 0x0A) ? '0' + nibble : 'A' + (nibble - 0x0A);
}

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

void transmit_footer() {
  transmit_bit(col_sums[0]);
  transmit_bit(col_sums[1]);
  transmit_bit(col_sums[2]);
  transmit_bit(col_sums[3]);
  transmit_bit(0);
}

void transmit_payload() {
  byte *p = payload;
  
  for(int row = 0; row < 10; row++) {
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
  transmit_payload();
  transmit_footer();
}

void set_payload_id(unsigned long id) {
  // TODO - for each nibble in `id`, convert to hex and put in payload
}

void set_payload_otp(uint8_t *otp) {
  // TODO - for each nibble in `otp`, convert to hex and put in payload
}

void calculate_row_sums() {
  // Take the sum of each nibble of payload (i.e. "row")
  byte *row = payload;
  for (short i = 0; i < 10; i++) {
    row_sums[i] = row[0] ^ row[1] ^ row[2] ^ row[3];
    row += 4;
  }
}

void calculate_col_sums() {
  // Take the sum of each bit in each nibble of payload (i.e. "col")
  byte *p = payload;
  for (short i = 0; i < 4; i++) {
    col_sums[i] = p[0] ^ p[4] ^ p[8] ^ p[12] ^ p[16] ^ p[20] ^ p[24] ^ p[28] ^ p[32] ^ p[36];
    p++;
  }
}

void setup() {
  unsigned long c;
  uint8_t *hash = NULL;

  // Grab nonce (counter, time, etc.)
  c = count();

  // Compute token
  Sha1.initHmac((const uint8_t*) F(OTP_KEY), OTP_KEY_LENGTH);
  Sha1.print(OTP_ID);
  Sha1.print(c);
  hash = Sha1.resultHmac();
  hash += hash[HASH_LENGTH-1] & 0x0F; // NOTE: PAYLOAD_NIBBLES_OTP <= 8 for this to be OK
  
  // Prepare payload
  set_payload_id(OTP_ID);
  set_payload_otp(hash);
  calculate_row_sums();
  calculate_col_sums();
  
  // Update EEPROM
  count(++c);
}

void loop() {
  // Transmit data indefinitely
  transmit_all();
}
