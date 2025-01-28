Emulation
======

This is *not* for the faint of heart, but emulating the ESP32, including a reverse-engineerd implementation of the closed-source ESP32 WiFi stack is possible!

Obviously, getting this setup is tremendously time-saving for development, and super helpful for debugging.

# How to do it

You need to get these components working:
1. a fork of ESP32's fork of QEMU that has (experimental) WiFi patches that add basic support for the reverse engineered ESP32 WiFi stack
2. a Git clone of ESP-IDF
3. the Arduino framework as an ESP-IDF component (to be able to build the Arduino-based Lightning Piggy project using ESP-IDF)
4. idf.py menuconfig # needs to have the appropriate configuration

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

## ESP-IDF

At the time of writing, the latest version of ESP-IDF that works with the Arduino framework for the ESP32 is [version 5.3.2](https://docs.espressif.com/projects/esp-idf/en/v5.3.2/esp32/get-started/index.html) so that's a good version to take.

Note that version prior to v5.2 have poor support for QEMU, although it's possible to backport the idf.py QEMU plugin to v5.1 or maybe even earlier versions.

Useful documentation is at:
- https://docs.espressif.com/projects/esp-idf/en/v5.3.2/esp32/get-started/index.html
- https://docs.espressif.com/projects/esp-idf/en/v5.3.2/esp32/get-started/linux-macos-setup.html

It's best to follow the official installation procedure, but here's the gist of it:

```
git clone -b v5.3.2 --recursive https://github.com/espressif/esp-idf.git esp-idf_v5.3.2
cd esp-idf_v5.3.2/
./install.sh esp32
. export.sh # you need to do this once in every shell where you want to call idf.py and related tools
```








