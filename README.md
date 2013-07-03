rfid-otp
========

`rfid-otp` is open-source hardware and firmware for secure EM4100-based RFID tokens.

Basic RFID tokens have a fixed payload (40 bytes) that they transmit every time they are powered up. `rfid-otp` uses an HMAC-OTP-based ([RFC 4226](http://tools.ietf.org/html/rfc4226) and, eventually, [RFC 6048](http://tools.ietf.org/html/rfc6238)) protocol for creating a cryptographically random payload. A portion of the payload remains constant to identify the token to the reader.

Arduino firmware requires [Cryptosuite](https://github.com/jkiv/Cryptosuite).
