# Lightning Piggy

[![License: LGPL-2.1](https://img.shields.io/github/license/LightningPiggy/lightning-piggy)](https://github.com/LightningPiggy/lightning-piggy/blob/master/LICENSE)
[![GitHub release](https://img.shields.io/github/v/release/LightningPiggy/lightning-piggy)](https://github.com/LightningPiggy/lightning-piggy/releases)
[![GitHub stars](https://img.shields.io/github/stars/LightningPiggy/lightning-piggy)](https://github.com/LightningPiggy/lightning-piggy/stargazers)
[![GitHub issues](https://img.shields.io/github/issues/LightningPiggy/lightning-piggy)](https://github.com/LightningPiggy/lightning-piggy/issues)
[![GitHub last commit](https://img.shields.io/github/last-commit/LightningPiggy/lightning-piggy)](https://github.com/LightningPiggy/lightning-piggy/commits/master)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/LightningPiggy/lightning-piggy)

Bitcoin Lightning piggybank using LNBits (for LNURLp) running on TTGO LilyGo ePaper hardware with ESP32 microcontroller.

See https://www.lightningpiggy.com/ and https://makers.bolt.fun/project/lightningpiggy

Main Source code
===========

The source code in Main/Main.ino works on both the TTGO LilyGo T5 ePaper 2.13 inch DEPG0213BN display and the (discontinued and more expensive) TTGO LilyGo T5 ePaper 2.66 inch DEPG0266BN display boards.

**With Arduino IDE**

- Arduino IDE version 2.3.5
- ESP32 Board Support version 3.2
- Preferences -> Compiler warnings: Default
- Tools -> Board -> ESP32 Arduino -> ESP32 Dev Module

Where: 

- Tools -> Upload Speed: 921600
- Tools -> CPU Frequency: 240Mhz
- Tools -> Flash Frequency: 80Mhz
- Tools -> Flash Mode: DIO (seems needed for the QEMU emulator)
- Tools -> Flash Size: 4MB (32Mb)
- Tools -> Partition Scheme: Custom (uses Main/partitions.csv)
- Tools -> Core Debug Level: Warn
- Tools -> PSRAM: Disabled
- Tools -> Port: /dev/ttyACM0

Make sure the Arduino IDE has permissions to access the serial port:

`sudo chmod -f 777 /dev/ttyACM* /dev/ttyUSB*`

**OR with Arduino CLI**

- Arduino CLI version 1.1.1-arch commit: fa6eafcbbea391eee
- LNBits v0.12.12 (tested with lnd version 0.18.99-beta and bitcoind 27.0.0)
- Debian Bookworm

Commands:

```
arduino-cli compile -u -v -t --libraries Main/libraries/ --fqbn esp32:esp32:esp32:PartitionScheme=custom,FlashMode=dio -p /dev/ttyACM0 Main/
```

**On the lnbits webpage:**

- activate the LNURLp extension
- click on the LNURLp extension
- click "NEW PAY LINK"
- untick "fixed amount"
- set minimum amount: 1
- set maximum amount: 100000000
- set currency to "satoshis"
- click "Advanced options"
- set "Comment maximum characters" to 128
- set "Webhook URL" to https://p.lightningpiggy.com/ (optional, for anonymous usage metrics)
- set a "Success message", like: Thanks for sending sats to my piggy

**Known issues:**
- There's an issue with the 2.13 inch GDEM0213B74 display, somehow the display becomes blank after updateWindow() calls. But we use the DEPG display, which is not affected.

How to install
==============

To install Lightning Piggy from the code base (rather than just using the latest release and using the web installer at https://lightningpiggy.github.io):

- Connect the device via USB
- Open /Main/Main.ino using Arduino Studio.
- Update values in config.h
- Copy the libraries used from C:\...\lightning-piggy\Main\libraries\ to your Arduino libraries folder (usually C:\Users\YourUsername\Documents\Arduino\libraries).
- Upload using Sketch > Upload.

To install the temporary Lightning Piggy Splash (used for shipping):

- Connect the device via USB
- Open /Splash/Splash.ino using Arduino Studio.
- Upload using Sketch > Upload.

How to release
==============

To make a new release available on the web installer:

- Update the version number in Constants.h
- Update CHANGELOG.md
- Make sure there are no uncommitted development changes (git diff; git diff --staged)
- Build and export the binaries with Arduino CLI (esp32 core 3.2.0, custom 4MB partition scheme, DIO flash mode — same settings as the build instructions above):

```
arduino-cli compile --libraries Main/libraries/ \
  --fqbn esp32:esp32:esp32:PartitionScheme=custom,FlashMode=dio \
  --export-binaries Main/
```

  This writes the binaries to `Main/build/esp32.esp32.esp32/`: `Main.ino.bin` (the app), `Main.ino.bootloader.bin`, `Main.ino.partitions.bin`, `Main.ino.merged.bin` (full 4MB flash image), plus `Main.ino.elf` and `Main.ino.map` (kept per-release for decoding crash backtraces with `addr2line`).

- Copy the build output into the web installer repo. `boot_app0.bin` is not produced by the build — it ships with the esp32 core (on Linux under `~/.arduino15/`, on macOS under `~/Library/Arduino15/`):

```
DST=~/sources/lightningpiggy.github.io/firmware/ttgo_lilygo_2.13_and_2.66_inch_epaper_6.x
CORE=~/.arduino15/packages/esp32/hardware/esp32/3.2.0
cp Main/build/esp32.esp32.esp32/Main.ino.bin            "$DST/"
cp Main/build/esp32.esp32.esp32/Main.ino.bootloader.bin "$DST/"
cp Main/build/esp32.esp32.esp32/Main.ino.partitions.bin "$DST/"
cp Main/build/esp32.esp32.esp32/Main.ino.merged.bin     "$DST/"
cp Main/build/esp32.esp32.esp32/Main.ino.elf            "$DST/"
cp Main/build/esp32.esp32.esp32/Main.ino.map            "$DST/"
cp "$CORE/tools/partitions/boot_app0.bin"               "$DST/"
```

- Check that the `boot_app0.bin` in the web installer repo matches the one shipped with the esp32 core:

```
md5sum "$CORE/tools/partitions/boot_app0.bin" "$DST/boot_app0.bin"
```

- Sanity-check that the partition table did not change between releases. It must stay byte-identical so that web-installs and OTA updates keep the existing `spiffs`/config partition at 0x390000 intact (a changed layout could brick configured devices):

```
git -C ~/sources/lightningpiggy.github.io diff --stat -- "$DST/Main.ino.partitions.bin"
# (no output = unchanged = good)
```

- Update the version number in `~/sources/lightningpiggy.github.io/manifests/manifest_ttgo_lilygo_2.13_and_2.66_inch_epaper_6.x.json`
- Commit and push the web installer repo. This is served via GitHub Pages, so pushing to `master` publishes the new firmware to the web installer (and to OTA) immediately:

```
pushd ~/sources/lightningpiggy.github.io/ ; git commit -a ; git push ; popd
```

- Tag the release on the lightning-piggy repo so it shows up under Releases (marked latest):

```
gh release create vX.Y.Z --target master --title "vX.Y.Z" --notes-file <changelog-notes> --latest
```

ESP32 emulation with QEMU (including WiFi!)
===================
See [these detailed instructions on ESP32 emulation with QEMU, including WiFi](Emulation.md).
