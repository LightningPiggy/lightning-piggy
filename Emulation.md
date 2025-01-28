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

## Arduino framework as an ESP-IDF component

To be able to build Arduino projects inside of ESP-IDF, you need to add the Arduino framework as an ESP-IDF component.

We might be able to avoid this step in the future by building with the Arduino IDE, and converting the resulting .bin files into a bootable image with merge_bin.

But as that didn't work out of the box, the way to crack this was first getting ESP-IDF WiFi examples working in QEMU, then getting Arduino WiFi examples working in QEMU, and finally getting the Lightning Piggy's Arduino code working in QEMU. 

Useful documentation is at:
- https://docs.espressif.com/projects/arduino-esp32/en/latest/esp-idf_component.html

```
idf.py create-project-from-example "espressif/arduino-esp32^3.1.1:hello_world"
cd hello_world/

idf.py build
[ $result -ne 0 ] && exit 1

cd build

esptool.py --chip esp32 merge_bin --output flash_image.bin --fill-flash-size=2MB --flash_mode dio --flash_freq 40m --flash_size 2MB 0x1000 bootloader/bootloader.bin 0x10000 main.bin 0x8000 partition_table/partition-table.bin

~/qemu_a159x36/build/qemu-system-xtensa -M esp32 -m 4M -drive file=flash_image.bin,if=mtd,format=raw -global driver=timer.esp32.timg,property=wdt_disable,value=true -nic user,model=esp32_wifi,hostfwd=tcp:127.0.0.1:8080-:8080 -nographic -serial tcp::5555,server &

sleep 0.5

nc localhost 5555

killall qemu-system-xtensa
cd ..
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

## idf.py menuconfig

You'll linker errors from  NetworkClientSecure.cpp regarding functions in ssl_client.cpp (such as stop_ssl_socket) unless you enable:
- Component config ---> mbedTLS ---> TLS Key Exchange Methods ---> Enable pre-shared-key ciphersuites ---> Enable PSK based ciphersuite modes

Optionally, consider enabling:
- Component config ---> LWIP ---> Enable LWIP Debug ---> check parts you're interested in



