**5.0.2**
- Reduce long press for configuration mode from 5 to 3 seconds
- Improve long press responsiveness during wifi connection

**5.0.1**
- Add "Restart" button to configuration webpage
- Fix boot loop caused by faulty sleep logic and failed assert()
- Reduce wifi connection timeout from 42 to 30 seconds
- If wifi connection fails, show error and go to sleep instead of starting Access Point configuration mode

**5.0.0**
- Add easy configuration method without reflashing every time. The device can become a wifi hotspot that users connect to for modifing the configuration options.
This "Configuration Mode" is started when the configuration is missing, when wifi connection fails, or by a long-press on the General Purpose (GPIO39) button.

**4.4.1**
- Add support for Malaysian Ringgit (MYR) fiat currency by prepending "RM"
- Ignore non-successful payments

**4.4.0**
- Fix bug that caused the device to stay awake as long as it failed to connect to wifi

**4.3.3**
- Fix bug that caused only the QR code and status (bottom) right to be displayed after 15 minutes of uptime

**4.3.2**
- Fix instant payments that broke due to LNBits WebSockets API change
- Add Spanish (es) boot slogans. Thanks, @Petrotronic!

**4.3.1**
- Reduce "Back up!" after tilt from 5 seconds to 2.

**4.3.0**
- Refresh display when device is tilted (if tilt sensor is present between pin 32 and 3V3)
- Fix erroneous usage of LNBits HTTPS port for non-LNBits hosts
- Fix crash when trying to request time from network when network is not connected
- Make compilation work on both arduino-esp 2.x and 3.x

**4.2.11**
- Add support for alternative LNBits HTTPS port configuration (instead of default 443)

**4.2.10**
- Fix bug that caused incoming payments not to show up when starting with an empty wallet

**4.2.7**
- Feed watchdog while retrying HTTPS connections, otherwise it might think it's hanging and restart the device.

**4.2.6**
- Only show battery info when battery is detected so no info implies no battery detected.

**4.2.5**
- Fix long number formatting

**4.2.3**
- Flag balance and payments for refresh when wifi (re)connects to clear display of any error messages

**4.2.2**
- Retry SSL (https) connections up to 3 times in case of failure

**4.2.1**
- Refresh balance and payments every 15 minutes (even if they should come in instantly) to be more resilient against server disconnections and network glitches
- Display all incoming payments for the wallet, including non-LNURL payments that came in through a regular invoice
- Don't show hardware info on-screen as it's not really needed and takes up valuable screen space
- Show battery as percentage instead of voltage
- Improve "Unknown Balance" error display
- Improve watchdog reset timing
- Fix potential websocket handler race condition

**4.0.5**
- Build unified firmware that supports both 2.13 and 2.66 inch ePaper display by detecting it at boot
- Migrate from GxEPD library to GxEPD2 library for much faster display updates
- Integrate U8g2_for_Adafruit_GFX library to support UTF-8 fonts (non-ascii characters such as öüäß)
- Add de_DE (German) and de_CH (Swiss German) boot slogans. Thanks @GABRI3L!
- Add da (Danish) boot slogans. Thanks @Jake21m!
- Rebuild boot logo to emphasize the lightning bolt in "Lightning Piggy" logo
- Place currency symbols before the fiat amount when appropriate
- Use proper fiat currency symbols (€,£,$,¥)
- Fix update notifications (they were broken in v3.x)

**3.1.3**
- Debugging release for a user that had issues with v3.x on one specific board and not the other.
- Built with ESP32 board support 1.0.6 like v2.x, instead of 2.0.14 like v3.0.x
- Update GxEPD library from 3.1.0 to 3.1.3
- Remove workarounds for GxEPD library to make display operations 20% faster on the 2.66 inch ePaper model

**3.0.3**
- Re-add lost "Wifi: <ssid>" on display to indicate ongoing wifi connection at boot
- Fix "ASSOC_LEAVE" leftover on display when disconnecting from wifi before sleep

**3.0.1**

- Add websocket for instant payment notifications while awake
- Improve stability and reduce backend load by avoiding frequent polls
- Reset awake timer after payment received in case another one comes in
- Improve timeout behavior in HTTPS fetcher
- Improve display code
- Remove unused libraries for smaller filesize and faster flashing
- Upgrade ESP32 Board Support version to 2.0.14
- Fix tilda character causing big blank space on display
- Fix partially blank timestamp on display
- Fix rare hang in chunked HTTPS fetcher
- Fix battery voltage glitch causing unnecessary sleeps
- Fix watchdog timeout configuration logic

**2.1.2**

- Translate English boot slogans to Dutch
- New feature: optional configurable "balance bias" to account for sats that are in cold storage
- New feature: optional configurable static LNURLp instead of fetching it from the LNBits API every time

**2.1.1**

- Add support for CHF currency symbol (Fr)
- Add support for German, Dutch and Spanish weekdays
- Add more boot slogans
- Fix display artifacts below horizontal line
- Don't show "NA" for unknown currency symbols
- Show only first character of weekdays

**2.0.0**

Extended battery life:

- Infinite wifi retry, infinite HTTP response waiting time and other long battery-draining exceptions are adressed with a "smart" watchdog.
- The wake-up frequency now adapts based on a seven-point profile correlated with battery voltage.

User experience improvements:

- Realtime balance and payment updates also happen under battery power after manual wakeups, thanks to new awake/sleep status decision logic.
- Notification code enhancements that help the user to easily troubleshoot any possible errors.
- On-screen indication when the device is sleeping vs awake.
- New on-screen software update visibility.
- New startup screen logo by Bitko.

Optional new features:

- Fiat balance and Bitcoin price with configurable fiat currency (USD, EUR, DKK,...)
- Random bootup "slogan" with configurable prelude
- Show last updated day and time

Removed features:

- Disabled HAL sensor value as it wasn't useful

Many thanks to @lonerookie for his awesome pull request full of new features and improvements!


**1.8.1**
- Make QR code a bit bigger on big displays
- Workaround partial display refresh on GDEM displays
- Don't send unnecessary HTTP headers

**1.8.0**
- Ensure compatibility with both 2.13 and 2.66 inch ePaper DEPG

**1.7.6**
- Detect when battery/no battery is connected from few/lots of VBAT (battery voltage) fluctuations.
- Display "NOBAT" instead of battery voltage when no battery is connected.

**1.7.2**
- Cleanly disconnect wifi before hibernate

**1.7.1**
- Show "Connecting to SSIDNAME..." at startup

**1.7.0**
- Show wifi signal strength (%)
- Reduce max font size of payment comments for esthetics
- Use partial display updates to increase speed and eliminate flickering

**1.6.0**
- Fix sat/sats plural
- Draw a line under the total sats amount
- Fix overflow for amounts bigger than 999999 sats
- Improve LOW BATTERY warning and cleanups

**1.5.0**
- Add health and status values to bottom right (battery voltage etc)
- Make sure second payment always fits
- Scale text based on available space
- Fix issue with empty comment payments (reported by Bitcoin3us)

**1.4.3**
- Speed up by eliminating unneeded lnbits URL call
- Speed up chunked reader with buffer
- Rewrite chunked HTTP parser
- Add update checker

**1.3.0**
- Add support for 2.13 inch DEPG ePaper board
- Add shake-to-update
- Add unified screen with total sats balance, LNURLp QR code for receiving, and 2 latest payments
- Add logo's at startup
