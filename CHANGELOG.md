**6.3.0**
- Update Adafruit BusIO from 1.15.0 to 1.17.0
- Update Adafruit GFX Library from 1.11.9 to 1.12.0
- Update ArduinoJson 7.3.0 to 7.4.1
- Update AsyncTCP from 3.3.3 to 3.3.8
- Update ESPAsyncWebServer from 3.6.2 to 3.7.6
- Update GxEPD2 from 1.5.6 to 1.6.3
- Update TFT_eSPI library from 2.2.20 to 2.5.4
- Update arduino-esp32 from 3.1.1 to 3.2.0 (implies ESP-IDF 5.3.2-282 to 5.4.1-1)
- Shorter access point name for configuration (Piggy Config)
- Implement automatic captive portal redirect
- Beautify configuration login page

**6.2.1**
- Fix white border bug on 2.66 inch ePaper display

**6.2.0**
- Show more info about what the device is doing at startup (connecting, time sync, webserver, fetching)
- Use standard NTP time sync (pool.ntp.org) for everything, eliminating dependency on https://timeapi.io
- Always display current time on display when powered on instead of only before sleeping
- Bring back translation of weekday names to local language
- Attempt wifi reconnection if connection is lost
- Add configurable "sleep mode" setting (feature request issue #35) so you can choose between "no_sleep", "custom_sleep" (default, 6 hours) and "endless_sleep" modes
- Add periodic daily (23h) restart to handle any unforeseen issues that might have occurred
- Add support for running the same build on both physical and virtual (QEMU) ESP32 devices without recompilation

**6.1.2**
- Fix memory leak that resulted in "Unknown Balance" appearing when using LNBits after 15 minutes (so on USB power)
- Improve boot salutations

**6.1.1**
- Avoid blank screen while fetching LNURLp from LNBits
- Fix unscannable QR code in case of LNBits backend without configured static receive code
- Don't show any QR code if no static receive code is configured and none can be fetched from wallet provider

**6.1.0**
- Nostr Wallet Connect: add support for NIP47 payment notifications for instant gratification; when a payment comes in, the new wallet balance(s) and updated list of payments are immediately displayed
- Fix issue #30 that would show "null" if no payment comment was provided (thanks for reporting, @jurjendevries !)

**6.0.1**
- Nostr Wallet Connect: add support for JSON-format payment descriptions (like coinos.io does it)
- Rate-limit fiat price fetching to 15 minutes
- Increase maximal configuration string length from 131 to 256 for long LNURLs
- Configuration page: order timezones alphabetically instead of by GMT offset

**6.0.0**
- Add support for Nostr Wallet Connect (NWC)
- Add NWC URL to configuration page
- Improve configuration page help text
- Add GMT offsets to timezones in configuration page for clarity
- Show IP address after connection to access point as client
- Short press on General Purpose (IO39) button refreshes the balance (like tilt sensor)
- Short press on General Purpose (IO39) button skips the wait during the logo/boot wisdom display
- Do full wifi scan and select the Access Point with the strongest signal

**5.0.6**
- Fix QR encoding of lightning addresses (like oink@demo.lnpiggy.com)

**5.0.5**
- Reduce wifi connection failure sleep from 8 to 4h
- Move to timeapi.io because worldtimeapi.org is down
- Re-enable web authentication of Configuration Mode

**5.0.4**
- Don't ignore "Always Run Webserver" setting

**5.0.3**
- Fetch fiat prices from Coingecko instead of Coindesk (because it's down)
- Add more websocket debugging output

**5.0.2**
- Reduce long press for configuration mode from 5 to 3 seconds
- Improve long press responsiveness during wifi connection
- Improve long press responsiveness during display draw
- Enable "Captive Portal" DNS to guide the user to the webpage in Configuration Mode
- Disable web authentication of Configuration Mode

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
