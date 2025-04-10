#!/bin/sh
# Run lightning-piggy in an ESP32 emulator with QEMU, including WiFi!
# See Emulation.md
#
# To generate the required Main/build files, click "Sketch" -> "Export Compiled Binary" in the Arduino IDE 2.x
# To use the Arduino IDE 1.x, see run_on_emulator_arduino_ide_1.x_builds.sh

SOFTWARE_DIR=Main/build/esp32.esp32.esp32/
APPNAME=$(find "$SOFTWARE_DIR" -iname "*ino.bin" )
BOOTLOADERNAME=$(find "$SOFTWARE_DIR" -iname "*ino.bootloader.bin" )
PARTITIONSNAME=$(find "$SOFTWARE_DIR" -iname "*ino.partitions.bin" )

BOOTAPP=~/.arduino15/packages/esp32/hardware/esp32/*/tools/partitions/boot_app0.bin

if [ -d config_partition/ ]; then
	echo "Folder config_partition/ found, creating LittleFS configuration filesystem..."
	~/sources/mklittlefs/mklittlefs -c config_partition/ -s 0x70000 config_partition.bin
	[ $? -ne 0 ] && echo "ERROR: could not create LittleFS configuration filesystem" && exit 1
fi


echo "Merging .bin files into bootable image..."
esptool.py --chip esp32 merge_bin --fill-flash-size=4MB --output flash_image.bin 0x1000 "$BOOTLOADERNAME" 0x8000 "$PARTITIONSNAME" 0xe000 $BOOTAPP 0x10000 $APPNAME 0x390000 config_partition.bin
# Or for ESP-IDF builds, use something like:
# esptool.py --chip esp32 merge_bin --fill-flash-size=4MB --output flash_image.bin 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/main.bin

[ $? -ne 0 ] && echo "ERROR: couldn't merge .bin files" && exit 1

echo "Both http://localhost:1080/ and http://localhost:1081/ are forwarded to the device. The former for Access Point mode, the latter in Station/Client mode."

# For GDB, add: -gdb tcp::3333 -S
# and then start gdb with "idf.py gdb"
# See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/qemu.html#debugging
~/sources/qemu_a159x36/build/qemu-system-xtensa -M esp32 -m 8M -drive file=flash_image.bin,if=mtd,format=raw -global driver=timer.esp32.timg,property=wdt_disable,value=true -nic user,model=esp32_wifi,hostfwd=tcp:127.0.0.1:1080-192.168.4.1:80,net=192.168.4.0/24,host=192.168.4.2,hostfwd=tcp:127.0.0.1:1081-192.168.4.15:80 -serial stdio

