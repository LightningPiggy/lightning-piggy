Emulation
======

It's not for the faint of heart, but it's possible, and tremendously useful and time-saving, to emulate the ESP32 (including WiFi!) on a regular development computer.

# How to do it

You need to get these components working:
1. a fork of ESP32's fork of QEMU that has (experimental) WiFi patches that add basic support for the reverse engineered ESP32 WiFi stack
2. a Git clone of ESP-IDF
3. the Arduino framework as an ESP-IDF component (to be able to build the Arduino-based Lightning Piggy project using ESP-IDF)

Additionally, you need:
- a Linux machine
- around 10GB of free disk space
- basic C/C++ compilation tools (build-essential)
- technical skills
- courage

