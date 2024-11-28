// Code for the Lightning Piggy running on the TTGO LilyGo T5 V2.3.1
//
// Tested with:
// - Arduino IDE version 2.3.3
// - ESP32 Board Support version 3.0.5
// - Preferences -> Compiler warnings: Default
// - Tools -> Board -> ESP32 Arduino -> ESP32 Dev Module
// - Tools -> Upload Speed: 115200
// - Tools -> CPU Frequency: 80Mhz
// - Tools -> Flash Frequency: 80Mhz
// - Tools -> Flash Mode: QIO
// - Tools -> Flash Size: 4MB (32Mb)
// - Tools -> Partition Scheme: Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)
// - Tools -> Core Debug Level: Warn
// - Tools -> PSRAM: Disabled
//
// See README.md for more tips and tricks.

#include <ArduinoJson.h>
#include <WebSocketsClient.h> // Needs to be here, otherwise compilation error...
#include <WiFiManager.h>

#include "logos.h"
#include "config.h"
#include "Constants.h"

WiFiManager wifiManager;

#define roundEight(x) (((x) + 8 - 1) & -8) // round up to multiple of 8

long lastUpdatedBalance = -UPDATE_BALANCE_PERIOD_MILLIS; // this makes it update when first run
int lastBalance = -NOT_SPECIFIED;
bool forceRefreshBalanceAndPayments = false;
int xBeforeLNURLp;

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Lightning Piggy " + getFullVersion());

  // turn on the green LED-IO12 on the LilyGo 2.66 inch, to show the board is on
  // it will turn off when the board hibernates
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH); // turn the LED on (HIGH is the voltage level)

  print_reset_reasons();
  print_wakeup_reason();

  setup_watchdog(); // do this as soon as possible, to workaround potential hangs, but not before turing on the power LED and printing debug info

  setup_display();
  displayVoltageWarning();
  showBootSlogan();
  showLogo(epd_bitmap_Lightning_Piggy, 104, 250, displayHeight() - 104, (displayWidth() - 250) / 2); // width and height are swapped because display rotation

  if (!loadConfigOrSetup())
  {
    Serial.println("Failed to load config or setup new one");
    delay(1000);

    ESP.restart();
    return;
  }

  displayWifiConnecting();
#ifndef DEBUG
  connectOrStartConfigAp(true);
  short_watchdog_timeout(); // after the long wifi connection stage, the next operations shouldn't take long
  displayWifiStrengthBottom();
#endif
  displayFetching();

  watchdogWasntTriggered();

  setup_interrupts(); // interrupts only make sense right before the loop
}

void loop()
{
  loop_interrupts();
  loop_websocket();

  // If there is no balance OR it has been a long time since it was refreshed, then refresh it
  if (lastBalance == -NOT_SPECIFIED || (millis() - lastUpdatedBalance) > UPDATE_BALANCE_PERIOD_MILLIS || forceRefreshBalanceAndPayments)
  {
    lastUpdatedBalance = millis();
    disconnectWebsocket();

    xBeforeLNURLp = showLNURLpQR(getLNURLp());
    xBeforeLNURLp = displayWidth() - roundEight(displayWidth() - xBeforeLNURLp);
    displayStatus(xBeforeLNURLp, false); // takes ~2000ms, which is too much to do with the websocket
    displayBalanceAndPayments(xBeforeLNURLp, forceRefreshBalanceAndPayments);
    forceRefreshBalanceAndPayments = false;
  }

  if (!isWebsocketConnected())
    connectWebsocket();

  feed_watchdog(); // Feed the watchdog regularly, otherwise it will "bark" (= reboot the device)
  if (!hibernateDependingOnBattery())
    delay(200);
}

void nextRefreshBalanceAndPayments()
{
  forceRefreshBalanceAndPayments = true;
}
