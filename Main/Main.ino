// Code for the Lightning Piggy running on the TTGO LilyGo 2.13 and 2.66 inch ePaper hardware.
// Tested with the DEPG display variants, not with GDEM.
//
// Tested with:
// - Arduino IDE version 1.8.13
// - ESP32 Board Support version 3.1.1
// - Preferences -> Compiler warnings: Default
// - Tools -> Board -> ESP32 Arduino -> ESP32 Dev Module
// - Tools -> Upload Speed: 921600
// - Tools -> CPU Frequency: 240Mhz
// - Tools -> Flash Frequency: 80Mhz
// - Tools -> Flash Mode: QIO
// - Tools -> Flash Size: 4MB (32Mb)
// - Tools -> Partition Scheme: Custom (uses Main/partitions.csv)
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
long lastHeap = 0;

int piggyMode = PIGGYMODE_INIT;

char* ssid = NULL;
char* password = NULL;

char* lnbitsHost = NULL;
char* lnbitsInvoiceKey = NULL;

char* lnbitsPort = NULL;
char* staticLNURLp = NULL;

char* btcPriceCurrencyChar = NULL;
char* balanceBias = NULL;
char* thousandsSeparator = NULL;
char* decimalSeparator = NULL;
char* bootSloganPrelude = NULL;
char* showSloganAtBoot = NULL;

char* timezone = NULL;
char* localeSetting = NULL;

char* alwaysRunWebserver = NULL;
char* checkUpdateHost = NULL;
char* timeServer = NULL;
char* timeServerPath = NULL;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Lightning Piggy " + getFullVersion());

    // turn on the green LED-IO12 on the LilyGo 2.66 inch, to show the board is on
    // it will turn off when the board hibernates
    pinMode(12, OUTPUT);
    digitalWrite(12, HIGH);   // turn the LED on (HIGH is the voltage level)

    print_reset_reasons();
    print_wakeup_reason();

    setup_watchdog();   // do this as soon as possible, to workaround potential hangs, but not before turing on the power LED and printing debug info
    setup_display();    // initialize display
    setup_config();     // load config file from filesystem (could be done later but makes sense to do it early)
    setup_webserver();  // prepare (don't start) webserver
    setup_interrupts(); // prepare button and tilt sensor handling

    watchdogWasntTriggered();
}

void loop() {
  feed_watchdog(); // Feed the watchdog regularly, otherwise it will "bark" (= reboot the device)
  loop_interrupts(); // handle keypress

  if (piggyMode == PIGGYMODE_INIT) {
    if (!displayVoltageWarning()) {
      if (strncmp(showSloganAtBoot,"YES", 3) != 0) {
        showLogo(epd_bitmap_Lightning_Piggy, 104, 250, displayHeight() - 104, (displayWidth() - 250) / 2); // width and height are swapped because display rotation
      } else {
        showBootSlogan();
      }
    } // if displayVoltageWarning() then no logo or slogan
    piggyMode = PIGGYMODE_SLEEP_BOOTSLOGAN;
  } else if (piggyMode == PIGGYMODE_SLEEP_BOOTSLOGAN) {
    if (doneWaitingForBootSlogan()) {
      if (hasMinimalConfig()) {
        piggyMode = PIGGYMODE_STARTING_STA;
      } else {
        piggyMode = PIGGYMODE_STARTING_AP;
      }
    } // else do nothing but wait
  } else if (piggyMode == PIGGYMODE_STARTING_STA) {
      displayFit("Wifi: " + String(ssid), 0, displayHeight()-smallestFontHeight, displayWidth(), displayHeight(), 1);
      stop_webserver();
      delay(1000);
      if (!connectWifiAsync()) piggyMode = PIGGYMODE_FAILED_STA;
  } else if (piggyMode == PIGGYMODE_WAITING_STA) {
    if (!keepWaitingWifi()) {
      if (wifiConnected()) {
        piggyMode = PIGGYMODE_STARTED_STA;
      } else {
        piggyMode = PIGGYMODE_FAILED_STA;
      }
    } // else keep waiting for wifi
  } else if (piggyMode == PIGGYMODE_STARTED_STA) {
    loop_interrupts();
    loop_websocket();
  
    // If there is no balance OR it has been a long time since it was refreshed, then refresh it
    if (lastBalance == -NOT_SPECIFIED || (millis() - lastUpdatedBalance) > UPDATE_BALANCE_PERIOD_MILLIS || forceRefreshBalanceAndPayments) {
      lastUpdatedBalance = millis();
      disconnectWebsocket();
  
      xBeforeLNURLp = showLNURLpQR(getLNURLp());
      loop_interrupts();
      xBeforeLNURLp = displayWidth()-roundEight(displayWidth()-xBeforeLNURLp);
      displayStatus(xBeforeLNURLp, false);  // takes ~2000ms, which is too much to do with the websocket
      loop_interrupts();
      displayBalanceAndPayments(xBeforeLNURLp, forceRefreshBalanceAndPayments);
      loop_interrupts();
      forceRefreshBalanceAndPayments = false;
      loop_interrupts();
      checkShowUpdateAvailable();
    }
  
    if (!isWebsocketConnected()) connectWebsocket();
    hibernateDependingOnBattery(); // go to sleep if that's necessary
    // Remain in this mode because we're waiting for websocket updates
  } else if (piggyMode == PIGGYMODE_FAILED_STA) {
    displayFit("I've been trying to connect to the wifi unsuccessfully for " + String(WIFI_CONNECT_TIMEOUT_SECONDS) + "s. Going to sleep for 8 hours...", 0, 40+5, displayWidth(), displayHeight()-smallestFontHeight-5, 1, false, false, true);
    hibernate(8*60*60);
  } else if (piggyMode == PIGGYMODE_STARTING_AP) {
    if (apstart_time == 0) {
      stop_webserver();
      displayFit("Starting wireless Access Point for configuration. This can take 20 seconds, please wait...", 0, 0, displayWidth(), displayHeight(), MAX_FONT);
      apstart_time = millis();
      wifi_init_softap();
    } else if (millis() - apstart_time < 17 * 1000) {
      // Wait until the AP is ready
      //Serial.print("*"); delay(200);
    } else {
      start_webserver();
      apstart_time = 0; // mark server as started
      piggyMode = PIGGYMODE_STARTED_AP;
      displayFit("Wireless Access Point started. Connect to the wifi called '" + String(ACCESS_POINT_SSID) + "' and open http://192.168.4.1/ in your webbrowser with username: " + String(WEBCONFIG_USERNAME) + " and password: " + String(WEBCONFIG_PASSWORD), 0, 0, displayWidth(), displayHeight(), MAX_FONT);
    }
  } else if (piggyMode == PIGGYMODE_STARTED_AP) {
    if (millis() > AWAKE_SECONDS_AS_ACCESS_POINT*1000) hibernateDependingOnBattery(); // go to sleep after a while, otherwise battery might drain
    // Nothing to do, just wait until the mode is changed.
  }

  if (millis() - lastHeap >= 2000) {
    Serial.printf("Free heap memory: %" PRIu32 " bytes\n", ESP.getFreeHeap());
    lastHeap = millis();
  }

}

void nextRefreshBalanceAndPayments() {
  forceRefreshBalanceAndPayments = true;
} 
