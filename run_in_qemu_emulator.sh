#!/bin/sh
# Run lightning-piggy in an ESP32 emulator with QEMU, including WiFi!
# See Emulation.md

SOFTWARE_DIR=/tmp/arduino_build_*
BOOTAPP=~/.arduino15/packages/esp32/hardware/esp32/2.0.17/tools/partitions/boot_app0.bin

esptool.py --chip esp32 merge_bin --fill-flash-size=4MB --output flash_image.bin 0x1000 $SOFTWARE_DIR/Main.ino.bootloader.bin 0x8000 $SOFTWARE_DIR/Main.ino.partitions.bin 0xe000 "$BOOTAPP" 0x10000 $SOFTWARE_DIR/Main.ino.bin

# Or for ESP-IDF builds, use something like:
# esptool.py --chip esp32 merge_bin --fill-flash-size=4MB --output flash_image.bin 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/main.bin
 
echo "Local port 1080 will be forwarded to port 80 on the emulated ESP32"
~/sources/qemu_a159x36/build/qemu-system-xtensa -M esp32 -m 4M -drive file=flash_image.bin,if=mtd,format=raw -global driver=timer.esp32.timg,property=wdt_disable,value=true -nic user,model=esp32_wifi,hostfwd=tcp:127.0.0.1:1080-:80 -serial stdio

