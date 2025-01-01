#include <esp_task_wdt.h>
#include <rtc_wdt.h>

// When to restart (s)
#define WDT_TIMEOUT 30
#define WDT_TIMEOUT_LONG 60

// "noinit" DRAM is not initialized during software restarts so it's the perfect place to keep this counter
// without the need to resort to limited-write-cycle flash storage such as EEPROM, NVS, flash,...
// See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/memory-types.html
__NOINIT_ATTR int watchdog_reboot_count;      // number of times in a row the device has been restarted by the watchdog

// Number of times the MAX_WATCHDOG_REBOOTS was reached, with long deepsleeps in between.
// Deepsleep doesn't preserve max_watchdog_reboot_count and watchdog reboot doesn't preserve rtc_max_watchdog_reboot_count.
// So both are needed to preserve this value across deepsleep and watchdog reboots.
__NOINIT_ATTR int max_watchdog_reboot_count;
RTC_DATA_ATTR int rtc_max_watchdog_reboot_count;

int count_watchdog_kicks = 0;
int lastFeed = millis();

// Count number of times the watchdog triggers a reboot
int updateWatchdogRebootCount() {
  if (rtc_get_reset_reason(0) == 9) { // if reboot reason was watchdog reset (RTCWDT_SYS_RESET)
    watchdog_reboot_count++;
    Serial.println("This is the " + String(watchdog_reboot_count) + "th reboot caused by the watchdog.");
  } else {
    watchdog_reboot_count = 0;
    Serial.println("This reboot was not caused by the watchdog, so watchdog_reboot_count = " + String(watchdog_reboot_count));
  }
  return watchdog_reboot_count;
}

void updateMaxWatchdogRebootCount() {
  if (rtc_get_reset_reason(0) == 5) { // if it was a DEEPSLEEP_RESET then read and restore max_watchdog_reboot_count because it's not saved across deepsleeps
    max_watchdog_reboot_count = rtc_max_watchdog_reboot_count;
  } else if ((rtc_get_reset_reason(0) != 9) && (rtc_get_reset_reason(0) != 5))  {
    max_watchdog_reboot_count = 0;
    Serial.print("Reboot reason was not RTCWDT_SYS_RESET and not DEEPSLEEP_RESET so max_watchdog_reboot_count chain is broken, ");
    Serial.println("resetting max_watchdog_reboot_count to " + String(max_watchdog_reboot_count));
  }
}

void watchdogWasntTriggered() {
  Serial.println("Everything went well without triggering watchdog so resetting max_watchdog_reboot_count to 0.");
  max_watchdog_reboot_count = 0;
  rtc_max_watchdog_reboot_count = 0;
}

void setup_watchdog() {
  Serial.println("Configuring WDT Watch Dog Timer...");
  //esp_task_wdt_add(NULL); // only needed for task watchdog timer
  enable_watchdog(WDT_TIMEOUT_LONG);

  updateMaxWatchdogRebootCount();
  Serial.println("This is the " + String(max_watchdog_reboot_count) + "nd time MAX_WATCHDOG_REBOOTS has been reached.");

  updateWatchdogRebootCount();
  if (watchdog_reboot_count >= MAX_WATCHDOG_REBOOTS) {
    longsleepAfterMaxWatchdogReboots();
  }
}

void longsleepAfterMaxWatchdogReboots() {
    max_watchdog_reboot_count++;
    rtc_max_watchdog_reboot_count = max_watchdog_reboot_count;  // save the value in a variable that is preserved across deepsleep

    int sleepHours = SLEEP_HOURS_AFTER_MAX_WATCHDOG_REBOOTS * max_watchdog_reboot_count;
    String errorMsg = "After " + String(watchdog_reboot_count) + " failure restarts, sleeping for ";
    errorMsg += String(sleepHours) + "h now. Happened " + String(max_watchdog_reboot_count) + " times in a row already.";
    Serial.println(errorMsg);
    // Wifi errors go up to y=40 so this error starts at y=41 and ends at displayHeight()-16 because "connecting to <SSID>" starts at displayHeight()-15
    displayFit(errorMsg, 0, 41, displayWidth(), displayHeight()-16, 4);

    // Unconditional hibernate because, in case of these frequent watchdog reboots,
    // it's probably better to go to sleep and try again some time later,
    // instead of retrying infinitely. Even while USB-powered.
    hibernate(sleepHours*60*60);
}

void enable_watchdog(int seconds) {
  int milliseconds = seconds*1000;
  int err = 0;

  // for RTC watchdog:
  // the code below was inspired by the espressive watchdog documentation and official examples
  rtc_wdt_protect_off();      //Disable RTC WDT write protection
  rtc_wdt_disable();

  // Left default length_of_reset_signal because it seems fine:
  //err = rtc_wdt_set_length_of_reset_signal(RTC_WDT_SYS_RESET_SIG, RTC_WDT_LENGTH_3_2us);
  //if (err) Serial.println("WARNING: rtc_wdt_set_length_of_reset_signal for " + String(RTC_WDT_LENGTH_3_2us) + " returned error: " + String(err));

  err = rtc_wdt_set_stage(RTC_WDT_STAGE0, RTC_WDT_STAGE_ACTION_RESET_SYSTEM);
  if (err) Serial.println("WARNING: rtc_wdt_set_stage(RTC_WDT_STAGE0, RTC_WDT_STAGE_ACTION_RESET_SYSTEM) returned error: " + String(err));

  err = rtc_wdt_set_time(RTC_WDT_STAGE0, milliseconds);
  if (err) Serial.println("WARNING: rtc_wdt_set_time for " + String(milliseconds) + "ms returned error: " + String(err));

  rtc_wdt_enable();           //Start the RTC WDT timer
  rtc_wdt_protect_on();

  unsigned int * timeout_ms = (unsigned int *) malloc(sizeof(unsigned int));
  err = rtc_wdt_get_timeout(RTC_WDT_STAGE0, timeout_ms);
  if (err) {
    Serial.println("WARNING: rtc_wdt_get_timeout for RTC_WDT_STAGE0 returned error: " + String(err));
  } else {
    unsigned int actual_timeout_ms = (*timeout_ms) * 22; // not sure why but the returned value seems ~21.95 times too low...
    Serial.println("rtc_wdt_get_timeout after setting it to " + String(milliseconds) + "ms returned: " + String(actual_timeout_ms) + "ms");
  }
  free(timeout_ms);

  if (rtc_wdt_is_on()) {
    Serial.println("rtc_wdt_is_on returns true");
  } else {
    Serial.println("rtc_wdt_is_on returns false");
  }
}

bool nextWatchdogRebootWillReachMax() {
  return (watchdog_reboot_count+1 >= MAX_WATCHDOG_REBOOTS);
}

void short_watchdog_timeout() {
  enable_watchdog(WDT_TIMEOUT);
}

void long_watchdog_timeout() {
  enable_watchdog(WDT_TIMEOUT_LONG);
}

// Meant to be called regularly, otherwise the watchdog will thing the device hung and reset it.
// Returns true so it can be used in if statements.
bool feed_watchdog() {
    //Serial.println("Feeding watchdog...");
    rtc_wdt_feed(); // for rtc watchdog
    //esp_task_wdt_reset(); // for task watchdog
    lastFeed = millis();
    count_watchdog_kicks++;
    return true;
}
