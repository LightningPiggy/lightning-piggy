ESP32 emulation with QEMU (including WiFi!)
======

This is *not* for the faint of heart, but emulating an ESP32 *with WiFi* is possible!

Needless to say, getting this set up reduces the typical code-compile-flash-test cycle by an order of magnitiude, and is super helpful for debugging.

Emulating the wifi stack is not officially supported by Espressive because it relies on a reverse-engineered implementation of the closed-source ESP32 WiFi stack.

It took quite a while figure it all out, investigate the issues, find workarounds for the weird bugs, simplify the process, get the code examples working, and finally, to get lightning-piggy (first ESP-IDF, then Arduino build) working in the emulator.

The following documentation puts all the pieces together in one logical process, and should be a huge time-saver for anyone who needs to get this working as well.

# Overview

## What works
- Running arduino-esp32 v3.1.1 based projects (including lightning-piggy, of course!)
- Running ESP-IDF v5.3.2 example projects
- WiFi: connection to emulated open access point "Open Wifi"
- WiFi: scanning and find a list of emulated open access points
- WiFi: access point mode (AP mode)
- Ethernet: example project from ESP-IDF v5.3.2 works
- Networking: running a TCP server by listening on a port
- Networking: DHCP, ARP, DNS, TCP, UDP (meaning HTTPS and websockets works)
- readAnalog() - seems to return some high value
- TFT display emulation
- ePaper display emulation


# Steps required

You need to get these components working:
1. a fork of ESP32's fork of QEMU that has (experimental) WiFi patches that add basic support for the reverse engineered ESP32 WiFi stack
2. the esptool.py from ESP-IDF
3. compiled binaries (.bin) of your project and the ESP32 bootloader
4. a script to combine the binaries and run on QEMU

Additionally, you need:
- a Linux machine
- around 10GB of free disk space
- basic C/C++ compilation tools (build-essential)
- technical skills
- some love for the commandline ;-)

## 1. Building QEMU with WiFi

Espressif maintains a [fork](https://github.com/espressif/qemu) of QEMU with ESP32 support, and there are several forks that have added a reverse-engineered WiFi stack to it:
- https://github.com/a159x36/qemu
- https://github.com/esp32-open-mac/qemu
- https://github.com/lcgamboa/qemu

Useful documentation is at:
- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/qemu.html
- https://github.com/espressif/esp-toolchain-docs/blob/main/qemu/esp32/README.md

For the Lightning Piggy, we started with a159x36's fork, because it works and seems the most up-to-date.

Then we applied a few patches to it to make the buttons (reset and General Purpose IO39) work by pressing 1 and 2 on the keyboard.
We also disabled a few unnecessary peripherals such as the servo.

To build it, you need to install some dependencies:

```
sudo apt install build-essential
sudo apt install libgcrypt20 libglib2.0-0 libpixman-1-0 libsdl2-2.0-0 libslirp0
sudo apt install libgcrypt20-dev # otherwise you'll get an error about "RSA"
```

Then clone and built it with:

```
git clone https://github.com/lighting-piggy/qemu
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
./qemu-system-xtensa # press CTRL-C to exit
```

## 2. esptool.py from ESP-IDF

You need a recent version of esptool.py from ESP-IDF for its merge_bin functionality.

If you're using the Arduino IDE with arduino-esp32 installed from the Library Manager, you should find it in ~/.arduino15/packages/esp32/tools/esptool_py/*/esptool.py 

Otherwise, you can install the ESP-IDF framework (explained below) and find it in the tools/ folder.

## 3. Compilation of your project

You can compile the project with the Arduino IDE 2.x, resulting in these files in Main/build if you choose "Sketch" -> "Export Compiled Binaries"
- Main.ino.bootloader.bin
- Main.ino.bin
- Main.ino.partitions.bin
- ~/.arduino15/packages/esp32/hardware/esp32/*/tools/partitions/boot_app0.bin

Or you can compile the project using ESP-IDF, typically resulting in these files:
- build/bootloader/bootloader.bin
- build/main.bin
- build/partition_table/partition-table.bin
  
### 3a Compiling with the Arduino IDE

Open the project ("Main.ino") in the Arduino IDE 2.x and the other files will open as well.

Follow the usual steps from the [README.md](README.md) to build it for a physical device, but instead of sending the sketch to your device using "Sketch" - "Upload", only compile it with "Sketch" - "Export Compiled Binary".

The resulting .bin files will be in Main/build/esp32.esp32.esp32/ such as:

```
user@arduino:~/Arduino/lightning-piggy$ ls -al Main/build/esp32.esp32.esp32/*bin
-rw-r--r-- 1 user user 1752048 Apr 10 11:03 Main/build/esp32.esp32.esp32/Main.ino.bin
-rw-r--r-- 1 user user   23440 Apr 10 11:03 Main/build/esp32.esp32.esp32/Main.ino.bootloader.bin
-rw-r--r-- 1 user user 4194304 Apr 10 11:03 Main/build/esp32.esp32.esp32/Main.ino.merged.bin
-rw-r--r-- 1 user user    3072 Apr 10 11:03 Main/build/esp32.esp32.esp32/Main.ino.partitions.bin
```

### 3b Compiling your Arduino project with ESP-IDF

To compile your Arduino project with ESP-IDF, you'll need:
- a Git clone of ESP-IDF
- the Arduino framework as an ESP-IDF component (to be able to build the Arduino-based Lightning Piggy project using ESP-IDF)
- idf.py menuconfig # needs to have the appropriate configuration

__Git clone of ESP-IDF__

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

__Arduino framework as an ESP-IDF component__

To be able to build Arduino projects inside of ESP-IDF, you need to add the Arduino framework as an ESP-IDF component.

We might be able to avoid this step in the future by building with the Arduino IDE, and converting the resulting .bin files into a bootable image with merge_bin.

But as that didn't work out of the box, the way to crack this was first getting ESP-IDF WiFi examples working in QEMU, then getting Arduino WiFi examples working in QEMU, and finally getting the Lightning Piggy's Arduino code working in QEMU. 

Useful documentation is at:
- https://docs.espressif.com/projects/arduino-esp32/en/latest/esp-idf_component.html

```
idf.py create-project-from-example "espressif/arduino-esp32^3.1.1:hello_world"
cd hello_world/

idf.py build
```

Now you can copy code from Arduino examples to main/main.cpp and run them to test.

IMPORTANT! The association with the access point will succeed BUT you won't receive any packets (DHCP replies, ARP replies, DNS replies) unless you do `WiFi.persistent(false);` before `WiFi.begin(ssid)`. This causes esp_wifi_set_storage(WIFI_STORAGE_RAM) to be called by the underlying ESP-IDF, and without it, it won't work.

A few useful ones to start:
- arduino-esp32/libraries/WiFi/examples/WiFiClient/WiFiScan.ino # note the hardcoded wifi networks (such as "Open Wifi") so use one of those
- arduino-esp32/libraries/WiFi/examples/WiFiClient/WiFiClient.ino # change .begin(ssid,password) to .begin(ssid) to connect to an open wifi
- arduino-esp32/libraries/WiFi/examples/SimpleWiFiServer/SimpleWiFiServer.ino # change the listening port to 8080 to make hostfwd work (see above) 

To get custom libraries to work, you need to follow the instructions.

For example, for arduinoWebsockets, copy the library to a new components/ folder and add a CMakeLists.txt file:

```
# cat components/arduinoWebsockets/CMakeLists.txt 
idf_component_register(SRCS "src/WebSocketsServer.cpp" "src/WebSocketsClient.cpp" "src/WebSockets.cpp"
                      INCLUDE_DIRS "src/"
                      REQUIRES arduino-esp32
                      )

project(arduinoWebsockets)
```

Then include it in the build with 'REQUIRES' in the main/CMakeLists.txt file:

```
idf_component_register(SRCS "Main.ino.cpp"
                INCLUDE_DIRS "."
                REQUIRES arduinoWebsockets)
```

__idf.py menuconfig__

You'll linker errors from  NetworkClientSecure.cpp regarding functions in ssl_client.cpp (such as stop_ssl_socket) unless you enable:
- Component config ---> mbedTLS ---> TLS Key Exchange Methods ---> Enable pre-shared-key ciphersuites ---> Enable PSK based ciphersuite modes

Optionally, consider enabling:
- Component config ---> LWIP ---> Enable LWIP Debug ---> check parts you're interested in



## 4. Script to combine the binaries and run on QEMU

Use something like this [run_on_emulator.sh script](run_on_emulator.sh) to combine all binaries into one bootable MTD "disk" image, boot it with QEMU and connect to the serial port.

When running it, you should see something like:

```
user@arduino:~/Arduino/lightning-piggy$ ./scripts/run_on_emulator.sh

esptool.py v4.8.1
Wrote 0x400000 bytes to file flash_image.bin, ready to flash to offset 0x0
Local port 1080 will be forwarded to port 80 on the emulated ESP32
Adding SPI flash device
ets Jul 29 2019 12:21:46

rst:0x1 (POWERON_RESET),boot:0x12 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0030,len:1184
load:0x40078000,len:13232
load:0x40080400,len:3028
entry 0x400805e4
[   348][D][esp32-hal-cpu.c:244] setCpuFrequencyMhz(): PLL: 480 / 2 = 240 Mhz, APB: 80000000 Hz
Starting Lightning Piggy 4.5.0|LILYGOT5V213|DEPG0213BN|Jan 28 2025 14:05:10
```

If the emulated device is listening on a port (like the SimpleWiFiServer.ino example), you can connect to it by browsing to [http://localhost:1080](http://localhost:1080).

# Credits

This work stands on the shoulders of many giants and open source software projects, but these stand out in particular
- Martin Johnson ([a159x36](https://github.com/a159x36)/[dzo](https://github.com/dzo)) for his QEMU-with-wifi fork
- the Lightning Piggy team for supporting this effort
