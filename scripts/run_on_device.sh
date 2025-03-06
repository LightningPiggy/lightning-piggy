#!/bin/sh
# Run lightning-piggy in an ESP32 emulator with QEMU, including WiFi!
# See Emulation.md

SOFTWARE_DIR=$(ls -1drt /tmp/arduino_build_* | tail -n 1)
APPNAME=$(find "$SOFTWARE_DIR" -iname "*ino.bin" )
BOOTLOADERNAME=$(find "$SOFTWARE_DIR" -iname "*ino.bootloader.bin" )
PARTITIONSNAME=$(find "$SOFTWARE_DIR" -iname "*ino.partitions.bin" )

BOOTAPP=~/.arduino15/packages/esp32/hardware/esp32/*/tools/partitions/boot_app0.bin

if [ -d config_partition/ ]; then
	echo "Folder config_partition/ found, creating LittleFS configuration filesystem..."
	~/sources/mklittlefs/mklittlefs -c config_partition/ -s 0x70000 config_partition.bin
	[ $? -ne 0 ] && echo "ERROR: could not create LittleFS configuration filesystem" && exit 1
fi

esptool.py --baud 921600 --after hard_reset write_flash 0x1000 "$BOOTLOADERNAME" 0x8000 "$PARTITIONSNAME" 0xe000 $BOOTAPP 0x10000 $APPNAME 0x390000 config_partition.bin

picocom -b 115200 /dev/ttyACM0
