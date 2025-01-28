Emulation
======

This is *not* for the faint of heart, but emulating the ESP32, including a reverse-engineerd implementation of the closed-source ESP32 WiFi stack is possible!

Obviously, getting this setup is tremendously time-saving for development, and super helpful for debugging.

# How to do it

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

## 2. esptool.py from ESP-IDF

You need a recent version of esptool.py from ESP-IDF for its merge_bin functionality.

If you're using the Arduino IDE with arduino-esp32 installed from the Library Manager, you should find it in ~/.arduino15/packages/esp32/tools/esptool_py/4.5.1/esptool.py 

Otherwise, you can install the ESP-IDF framework (explained below) and find it in the tools/ folder.

## 3. Compilation of your project

You can compile the project with the Arduino IDE, resulting in these files:
- Main.ino.bootloader.bin
- Main.ino.bin
- Main.ino.partitions.bin
- ~/.arduino15/packages/esp32/hardware/esp32/2.0.17/tools/partitions/boot_app0.bin

Or you can compile the project using ESP-IDF, typically resulting in these files:
- build/bootloader/bootloader.bin
- build/main.bin
- build/partition_table/partition-table.bin
  
### 3a Compiling with the Arduino IDE

Open the project ("Main.ino") in the Arduino IDE and the other files will open as well.

Follow the usual steps from the [README.md](README.md) to build it for a physical device, but instead of sending the sketch to your device using "Sketch" - "Upload", only compile it with "Sketch" - "Verify/Compile".

The resulting .bin files will be in /tmp/arduino_build_*/*.bin such as:

```
user@arduino:~$ ls -al /tmp/arduino_build_*/*bin
-rw-r--r-- 1 user user 1158480 Jan 28 11:59 /tmp/arduino_build_833289/Main.ino.bin
-rw-r--r-- 1 user user   17536 Jan 28 11:59 /tmp/arduino_build_833289/Main.ino.bootloader.bin
-rw-r--r-- 1 user user    3072 Jan 28 11:59 /tmp/arduino_build_833289/Main.ino.partitions.bin
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

Use something like the following script to combine all binaries into one bootable MTD "disk" image, boot it with QEMU and connect to the serial port:

```
SOFTWARE_DIR=/tmp/arduino_build_*

esptool.py --chip esp32 merge_bin --fill-flash-size=4MB --output=flash_image.bin 0x1000 "$SOFTWARE_DIR"/Main.ino.bootloader.bin 0x8000 "$SOFTWARE_DIR"/Main.ino.partitions.bin 0xe000 ~/.arduino15/packages/esp32/hardware/esp32/2.0.17/tools/partitions/boot_app0.bin 0x10000 "$SOFTWARE_DIR"/Main.ino.bin

echo "Local port 10080 will be forwarded to port 80 on the emulated ESP32"
/home/user/sources/qemu_a159x36/build/qemu-system-xtensa -M esp32 -m 4M -drive file=flash_image.bin,if=mtd,format=raw -global driver=timer.esp32.timg,property=wdt_disable,value=true -nic user,model=esp32_wifi,hostfwd=tcp:127.0.0.1:18080-:18080 -nographic -serial tcp::5555,server &

sleep 0.5

echo "Connecting to console. Press CTRL-C to stop the emulation..."
nc localhost 5555

killall qemu-system-xtensa
```

When running it, you should see something like:

```
user@arduino:~/Arduino/lightning-piggy$ ./run_qemu.sh

esptool.py v4.8.1
SHA digest in image updated
Wrote 0x400000 bytes to file flash_image.bin, ready to flash to offset 0x0
Local port 10080 will be forwarded to port 80 on the emulated ESP32
QEMU 9.0.0 monitor - type 'help' for more information
qemu-system-xtensa: -serial tcp::5555,server: info: QEMU waiting for connection on: disconnected:tcp:0.0.0.0:5555,server=on
Connecting to console. Press CTRL-C to stop the emulation...
Adding SPI flash device
(qemu) ets Jul 29 2019 12:21:46

rst:0x1 (POWERON_RESET),boot:0x12 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0030,len:1184
load:0x40078000,len:13232
load:0x40080400,len:3028
entry 0x400805e4
[   333][D][esp32-hal-cpu.c:244] setCpuFrequencyMhz(): PLL: 480 / 2 = 240 Mhz, APB: 80000000 Hz
Starting Lightning Piggy 4.5.0|LILYGOT5V213|DEPG0213BN|Jan 28 2025 14:05:10
```

If the emulated device is listening on a port (like the SimpleWiFiServer.ino example), you can connect to it by browsing to `http://localhost:10080`.
