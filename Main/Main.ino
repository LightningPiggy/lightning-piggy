// Code for the Lightning Piggy running on the TTGO LilyGo 2.13 and 2.66 inch ePaper hardware.
// Tested with the DEPG display variants, not with GDEM.
//
// Tested with:
// - Arduino IDE version 1.8.13
// - ESP32 Board Support version 2.0.17
// - Preferences -> Compiler warnings: Default
// - Tools -> Board -> ESP32 Arduino -> ESP32 Dev Module
// - Tools -> Upload Speed: 921600
// - Tools -> CPU Frequency: 240Mhz
// - Tools -> Flash Frequency: 80Mhz
// - Tools -> Flash Mode: QIO
// - Tools -> Flash Size: 4MB (32Mb)
// - Tools -> Partition Scheme: Default 4MB with spiffs (1.2MB APP, 1.5MB SPIFFS)
// - Tools -> Core Debug Level: Warn
// - Tools -> PSRAM: Disabled
// - Tools -> Port: /dev/ttyACM0
//
// See README.md for more tips and tricks.

#include <ArduinoJson.h>
#include <WebSocketsClient.h> // Needs to be here, otherwise compilation error...

#include "logos.h"
#include "config.h"
#include "Constants.h"

#define roundEight(x) (((x) + 8 - 1) & -8) // round up to multiple of 8

long lastUpdatedBalance = -UPDATE_BALANCE_PERIOD_MILLIS;  // this makes it update when first run
int lastBalance = -NOT_SPECIFIED;
bool forceRefreshBalanceAndPayments = false;
int xBeforeLNURLp;
long apstart_time  = 0;

int piggyMode = PIGGYMODE_INIT;

char* ssid = NULL;
char* password = NULL;
char* lnbitsHost = NULL;
char* lnbitsPort = NULL;
char* lnbitsInvoiceKey = NULL;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Lightning Piggy " + getFullVersion());

    // turn on the green LED-IO12 on the LilyGo 2.66 inch, to show the board is on
    // it will turn off when the board hibernates
    pinMode(12, OUTPUT);
    digitalWrite(12, HIGH);   // turn the LED on (HIGH is the voltage level)

    print_reset_reasons();
    print_wakeup_reason();

    setup_watchdog(); // do this as soon as possible, to workaround potential hangs, but not before turing on the power LED and printing debug info

    setup_display();
    displayVoltageWarning();
    showBootSlogan();
    showLogo(epd_bitmap_Lightning_Piggy, 104, 250, displayHeight() - 104, (displayWidth() - 250) / 2); // width and height are swapped because display rotation

    setup_config();

    setup_webserver();

    setup_interrupts();

    watchdogWasntTriggered();
}

void loop() {
  if (piggyMode == PIGGYMODE_INIT) {
    if (ssid != NULL && strnlen(ssid, MAX_CONFIG_LENGTH) > 0) { // TODO: call some sort of "hasMinimalConfig" instead that also checks for minimal wallet connection settings
      piggyMode = PIGGYMODE_STARTING_STA;
    } else {
      piggyMode = PIGGYMODE_STARTING_AP;
    }
  } else if (piggyMode == PIGGYMODE_STARTING_STA) {
      displayWifiConnecting();
      #ifndef DEBUG
      stop_webserver();
      delay(1000);
      if (connectWifi()) {
        start_webserver(); // TODO: make this dependent on a configuration, default off
        short_watchdog_timeout(); // after the long wifi connection stage, the next operations shouldn't take long
        displayWifiStrengthBottom();
        #endif
        displayFetching();
        piggyMode = PIGGYMODE_STARTED_STA;
      } else {
        piggyMode = PIGGYMODE_FAILED_STA;
      }
  } else if (piggyMode == PIGGYMODE_STARTED_STA) {
    loop_interrupts();
    loop_websocket();
  
    // If there is no balance OR it has been a long time since it was refreshed, then refresh it
    if (lastBalance == -NOT_SPECIFIED || (millis() - lastUpdatedBalance) > UPDATE_BALANCE_PERIOD_MILLIS || forceRefreshBalanceAndPayments) {
      lastUpdatedBalance = millis();
      disconnectWebsocket();
  
      xBeforeLNURLp = showLNURLpQR(getLNURLp());
      xBeforeLNURLp = displayWidth()-roundEight(displayWidth()-xBeforeLNURLp);
      displayStatus(xBeforeLNURLp, false);  // takes ~2000ms, which is too much to do with the websocket
      displayBalanceAndPayments(xBeforeLNURLp, forceRefreshBalanceAndPayments);
      forceRefreshBalanceAndPayments = false;
  
      checkShowUpdateAvailable();
    }
  
    if (!isWebsocketConnected()) connectWebsocket();
    if (!hibernateDependingOnBattery()) delay(200);
    // Remain in this mode because we're waiting for websocket updates
  } else if (piggyMode == PIGGYMODE_FAILED_STA) {
      displayWifiIssue(WIFI_CONNECT_TIMEOUT_SECONDS);
      // TODO: record the time of this display message, and after a while, go to AP mode?
      hibernateDependingOnBattery();
      piggyMode = PIGGYMODE_STARTING_AP; // If it hasn't gone to sleep, start the Access Point for configuration
  } else if (piggyMode == PIGGYMODE_STARTING_AP) {
    if (apstart_time == 0) {
      stop_webserver();
      apstart_time = millis();
      wifi_init_softap();
    } else if (millis() - apstart_time < 16 * 1000) {
      // Wait until the AP is ready
      Serial.print("*"); delay(200);
    } else {
      start_webserver();
      apstart_time = 0; // mark server as started
      piggyMode = PIGGYMODE_STARTED_AP;
    }
  } else if (piggyMode == PIGGYMODE_STARTED_AP) {
    //Serial.println("TODO: show PIGGYMODE_STARTED_AP on display");
  } else if (piggyMode == PIGGYMODE_FAILED_AP) {
    //Serial.println("TODO: show PIGGYMODE_FAILED_AP on display");
  }
  
  feed_watchdog(); // Feed the watchdog regularly, otherwise it will "bark" (= reboot the device)
}

void nextRefreshBalanceAndPayments() {
  forceRefreshBalanceAndPayments = true;
} 
