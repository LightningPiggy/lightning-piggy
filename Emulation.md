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

## Building QEMU with WiFi

Espressif maintains a [fork](https://github.com/espressif/qemu) of QEMU with ESP32 support, and there are several forks that have added a reverse-engineered WiFi stack to it:
- https://github.com/a159x36/qemu
- https://github.com/esp32-open-mac/qemu
- https://github.com/lcgamboa/qemu

Useful documentation is at:
- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/qemu.html
- https://github.com/espressif/esp-toolchain-docs/blob/main/qemu/esp32/README.md

The tradeoff's aren't very clear here, but a159x36's fork works and seems the most recently updated.

To build it, you need to install some dependencies:

sudo apt install build-essential
sudo apt install libgcrypt20 libglib2.0-0 libpixman-1-0 libsdl2-2.0-0 libslirp0
sudo apt install libgcrypt20-dev # otherwise you'll get an error about "RSA"


something like:

```
git clone https://github.com/a159x36/qemu
cd qemu/
mkdir build
cd build/

# gcrypt is for RSA
# slirp is for user networking
# sdl is for display emulation
./configure --target-list=xtensa-softmmu \
    --enable-gcrypt \
    --enable-slirp  \
    --enable-debug  \
    --enable-sanitizers \
    --enable-sdl
make -j3 # 3 is the number of CPUs in your machine
```

After building, you should have the emulator ready to go.

Give it a test with:

```
./qemu-system-xtensa &
sleep 10
pkill -f qemu
```


