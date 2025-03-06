SOFTWARE_DIR=$(ls -1drt /tmp/arduino_build_* | tail -n 1)
APPNAME=$(find "$SOFTWARE_DIR" -iname "*.ino.elf" )

if [ ! -z "$APPNAME" ]; then
	echo "Running addr2line on $APPNAME..."
	#~/.espressif/tools/xtensa-esp-elf/esp-13.2.0_20240530/xtensa-esp-elf/bin/xtensa-esp-elf-addr2line -e ~/sources/lightningpiggy.github.io/firmware/ttgo_lilygo_2.13_and_2.66_inch_epaper_6.x/Main.ino.elf $@
	~/.espressif/tools/xtensa-esp-elf/esp-13.2.0_20240530/xtensa-esp-elf/bin/xtensa-esp-elf-addr2line -e "$APPNAME" $@
else
	echo "ERROR: could not find *.ino.elf file!"
fi
