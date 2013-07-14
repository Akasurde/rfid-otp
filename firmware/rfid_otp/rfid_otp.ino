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

#define PAYLOAD_NIBBLES_ID  3 //!< budget for ID in payload
#define PAYLOAD_NIBBLES_OTP 7 //!< budget for OTP in payload (<= 8)

/*
 On attiny85's pins (http://tronixstuff.wordpress.com/2011/11/23/using-an-attiny-as-an-arduino/):
   - digital pin zero is physical pin five (also PWM)
   - digital pin one is physical pin six (also PWM)
   - analogue input two is physical pin seven
   - analogue input three is physical pin two
   - analogue input four is physical pin three
*/

#define COIL_PIN 0    //!< Pin used to drive the coil
#define BIT_DELAY 256 //!< delay between each transistion in microseconds (Manchester encoding), 64 cycles-per-bit

// TODO store bits as bits, not bytes
byte payload[40] = { 0 }; //!< data payload (D00-39) where D00-D03 is /not/ considered special
byte row_sums[10] = { 0 }; //!< row parity bits (P0-9)
byte col_sums[4] = { 0 }; //!< column parity bits (PC0-3)

byte *payload_id = payload; //!< payload offset for ID
byte *payload_otp = payload + 4*PAYLOAD_NIBBLES_ID; //!< payload offset for OTP

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

//!< Transmit a bit (Manchester Encoding, as per IEEE 802.3)
void transmit_bit(byte data) {
  if (data & 0x01) {
    // Transmit 1 (low->high)
    digitalWrite(COIL_PIN, LOW);
    delayMicroseconds(BIT_DELAY);
    digitalWrite(COIL_PIN, HIGH);
    delayMicroseconds(BIT_DELAY);
  }
  else {
    // Transmit 0 (low->high)
    digitalWrite(COIL_PIN, HIGH);
    delayMicroseconds(BIT_DELAY);
    digitalWrite(COIL_PIN, LOW);
    delayMicroseconds(BIT_DELAY);
  }
}

//!< Transmit EM4100 header (9 high bits)
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

//!< Transmit EM4100 payload (40 bits)
void transmit_payload() {
  byte *p = payload;
  
  for(int row = 0; row < 10; row++) {
    // Send next nibble
    transmit_bit(p[0]);
    transmit_bit(p[1]);
    transmit_bit(p[2]);
    transmit_bit(p[3]);
    p += 4;
    // Send checksum
    transmit_bit(row_sums[row]);
  }
}

//!< Transmit EM4100 footer (column sums and 0 end-bit)
void transmit_footer() {
  transmit_bit(col_sums[0]);
  transmit_bit(col_sums[1]);
  transmit_bit(col_sums[2]);
  transmit_bit(col_sums[3]);
  transmit_bit(0);
}

//!< Transmit EM4100 token
void transmit_all() {
  transmit_header();
  transmit_payload();
  transmit_footer();
}

//!< Set ID in payload (least-significant PAYLOAD_NIBBLES_ID nibbles of id)
void set_payload_id(unsigned long id) {
  byte *p = payload_id + (4*PAYLOAD_NIBBLES_ID-1);
  for (int i = 0; i < 4*PAYLOAD_NIBBLES_ID; i++) {
    *p = (id>>i) & 0x01;
    p--;
  }
}

//!< Set OTP in payload (first PAYLOAD_NIBBLES_OTP nibbles of otp)
void set_payload_otp(uint8_t *otp) {
  byte nib;
  byte *p = payload_otp;
  
  for(int n = 0; n < PAYLOAD_NIBBLES_OTP; n++) {
    nib = otp[n/2];
    if (!(n%2)) nib >>= 4;
    nib &= 0x0F;
    
    // Store nibble in payload
    p[0] = nib>>3 & 0x01;
    p[1] = nib>>2 & 0x01;
    p[2] = nib>>1 & 0x01;
    p[3] = nib & 0x01;
    p += 4;
  }
}

void calculate_row_sums() {
  // Take the sum of each nibble of payload (i.e. "row")
  byte *row = payload;
  for (int i = 0; i < 10; i++) {
    row_sums[i] = row[0] ^ row[1] ^ row[2] ^ row[3];
    row += 4;
  }
}

void calculate_col_sums() {
  // Take the sum of each bit in each nibble of payload (i.e. "col")
  byte *p = payload;
  for (int i = 0; i < 4; i++) {
    col_sums[i] = p[0] ^ p[4] ^ p[8] ^ p[12] ^ p[16] ^ p[20] ^ p[24] ^ p[28] ^ p[32] ^ p[36];
    p++;
  }
}

void setup() {
  unsigned long c;
  uint8_t *hash = NULL;
  
  // Get counter
  c = count();

  // Compute token
  Sha1.initHmac((const uint8_t*) F(OTP_KEY), OTP_KEY_LENGTH);
  Sha1.print((uint16_t) OTP_ID);
  Sha1.print(c);
  hash = Sha1.resultHmac();
  hash += hash[HASH_LENGTH-1] & 0x0F;
  
  // Prepare payload
  set_payload_id(OTP_ID);
  set_payload_otp(hash);
  calculate_row_sums();
  calculate_col_sums();
  
  // Update counter
  count(++c);
}

void loop() {
  // Transmit indefinitely
  transmit_all();
}
