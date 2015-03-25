rfid-otp
========

![Project status: abandoned](https://img.shields.io/badge/project status-abandoned-red.svg)

`rfid-otp` is open-source hardware and firmware for secure EM4100-based RFID tokens.

Basic RFID tokens have a fixed payload that they transmit every time they are powered up. `rfid-otp` uses an HMAC-OTP protocol (based on [RFC 4226](http://tools.ietf.org/html/rfc4226) and in the future [RFC 6048](http://tools.ietf.org/html/rfc6238)) for creating a cryptographically random payload in order to prevent spoofing.

Arduino firmware requires [Cryptosuite](https://github.com/jkiv/Cryptosuite).
