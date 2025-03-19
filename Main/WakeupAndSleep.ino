// Task watchdog, ESP.restart(), assert() fails and crashes causes this reset reason:
// rst:0xc (SW_CPU_RESET),boot:0x17 (SPI_FAST_FLASH_BOOT)
// 20:45:32.576 -> CPU0 reset reason:
// 20:45:32.576 -> SW_CPU_RESET
// 20:45:32.576 -> Software reset CPU
// 20:45:32.576 -> CPU1 reset reason:
// 20:45:32.576 -> SW_CPU_RESET
// 20:45:32.576 -> Software reset CPU

// RTC Watchdog Timer (RTC_WDT) restarts:
// 20:50:45.192 -> CPU0 reset reason:
// 20:50:45.192 -> RTCWDT_SYS_RESET
// 20:50:45.192 -> RTC Watch dog Reset digital core
// 20:50:45.192 -> CPU1 reset reason:
// 20:50:45.192 -> EXT_CPU_RESET
// 20:50:45.192 -> for APP CPU, reseted by PRO CPU
// 20:50:45.192 -> Wakeup was not caused by deep sleep, wakeup reason: 0

// Physical reset button pressed OR software uploaded using firmware flasher:
// 13:37:19.815 -> rst:0x1 (POWERON_RESET),boot:0x17 (SPI_FAST_FLASH_BOOT)
// 13:37:19.815 -> configsip: 0, SPIWP:0xee
// 21:09:28.165 -> CPU0 reset reason:
// 21:09:28.165 -> POWERON_RESET
// 21:09:28.165 -> Vbat power on reset
// 21:09:28.165 -> CPU1 reset reason:
// 21:09:28.165 -> EXT_CPU_RESET
// 21:09:28.165 -> for APP CPU, reseted by PRO CPU

// Physical off-on sliding switch:
// 21:10:44.795 -> CPU0 reset reason:
// 21:10:44.795 -> RTCWDT_RTC_RESET
// 21:10:44.795 -> RTC Watch dog reset digital core and rtc module
// 21:10:44.795 -> CPU1 reset reason:
// 21:10:44.795 -> EXT_CPU_RESET
// 21:10:44.795 -> for APP CPU, reseted by PRO CPU

// Wakeup from deepsleep by timer:
// 22:05:08.530 -> CPU0 reset reason:
// 22:05:08.530 -> DEEPSLEEP_RESET
// 22:05:08.530 -> Deep Sleep reset digital core
// 22:05:08.530 -> CPU1 reset reason:
// 22:05:08.530 -> EXT_CPU_RESET
// 22:05:08.530 -> for APP CPU, reseted by PRO CPU
// 22:05:08.530 -> Wakeup from sleep count: 1
// 22:05:08.530 -> Wakeup caused by timer


#include "driver/rtc_io.h"  // for rtc_gpio_pullup_dis and rtc_gpio_pulldown_en
#include <rom/rtc.h>        // for rtc_get_reset_reason

#define BUTTON_PIN_BITMASK 4294967296 // 2^32 means GPIO32

// "Data in RTC memory is initialised whenever the SoC restarts, except when waking from deep sleep.
// When waking from deep sleep, the values which were present before going to sleep are kept."
// if the device wakes up from sleep, wakeup_count is preserved and incremented.
// if the device wakes up from something else, like a watchdog trigger, the wakeup_count is reset
RTC_DATA_ATTR int wakeup_count;

long lastHibernateCheck = -HIBERNATE_CHECK_PERIOD_MILLIS;

long lastPaymentReceivedMillis = 0;

String print_reset_reasons() {
    Serial.println("CPU0 reset reason:");
    print_reset_reason(rtc_get_reset_reason(0));
    verbose_print_reset_reason(rtc_get_reset_reason(0));

    Serial.println("CPU1 reset reason:");
    print_reset_reason(rtc_get_reset_reason(1));
    verbose_print_reset_reason(rtc_get_reset_reason(1));

    return "cpu 0: " + String(rtc_get_reset_reason(0)) + " cpu 1: " + String(rtc_get_reset_reason(1));
}

void print_reset_reason(int reason) {
  switch ( reason)
  {
    case 1 : Serial.println ("POWERON_RESET");break;          /**<1,  Vbat power on reset, after flashing firmware or pushing reset button */
    case 3 : Serial.println ("SW_RESET");break;               /**<3,  Software reset digital core*/
    case 4 : Serial.println ("OWDT_RESET");break;             /**<4,  Legacy watch dog reset digital core*/
    case 5 : Serial.println ("DEEPSLEEP_RESET");break;        /**<5,  Deep Sleep reset digital core, after wakeup from hibernate */
    case 6 : Serial.println ("SDIO_RESET");break;             /**<6,  Reset by SLC module, reset digital core*/
    case 7 : Serial.println ("TG0WDT_SYS_RESET");break;       /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8 : Serial.println ("TG1WDT_SYS_RESET");break;       /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9 : Serial.println ("RTCWDT_SYS_RESET");break;       /**<9,  RTC Watch dog Reset digital core*/
    case 10 : Serial.println ("INTRUSION_RESET");break;       /**<10, Instrusion tested to reset CPU*/
    case 11 : Serial.println ("TGWDT_CPU_RESET");break;       /**<11, Time Group reset CPU*/
    case 12 : Serial.println ("SW_CPU_RESET");break;          /**<12, Software reset CPU: happens after ESP.reset() as well as Task Watchdog Timer (TWDT) reset */
    case 13 : Serial.println ("RTCWDT_CPU_RESET");break;      /**<13, RTC Watch dog Reset CPU*/
    case 14 : Serial.println ("EXT_CPU_RESET");break;         /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : Serial.println ("RTCWDT_BROWN_OUT_RESET");break;/**<15, Reset when the vdd voltage is not stable*/
    case 16 : Serial.println ("RTCWDT_RTC_RESET");break;      /**<16, RTC Watch dog reset digital core and rtc module, after sliding switch off-on */
    default : Serial.println ("NO_MEAN");
  }
}

void verbose_print_reset_reason(int reason) {
  switch ( reason)
  {
    case 1  : Serial.println ("Vbat power on reset");break;
    case 3  : Serial.println ("Software reset digital core");break;
    case 4  : Serial.println ("Legacy watch dog reset digital core");break;
    case 5  : Serial.println ("Deep Sleep reset digital core");break;
    case 6  : Serial.println ("Reset by SLC module, reset digital core");break;
    case 7  : Serial.println ("Timer Group0 Watch dog reset digital core");break;
    case 8  : Serial.println ("Timer Group1 Watch dog reset digital core");break;
    case 9  : Serial.println ("RTC Watch dog Reset digital core");break;
    case 10 : Serial.println ("Instrusion tested to reset CPU");break;
    case 11 : Serial.println ("Time Group reset CPU");break;
    case 12 : Serial.println ("Software reset CPU");break;
    case 13 : Serial.println ("RTC Watch dog Reset CPU");break;
    case 14 : Serial.println ("for APP CPU, reseted by PRO CPU");break;
    case 15 : Serial.println ("Reset when the vdd voltage is not stable");break;
    case 16 : Serial.println ("RTC Watch dog reset digital core and rtc module");break;
    default : Serial.println ("NO_MEAN");
  }
}

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
String print_wakeup_reason(){
  Serial.println("Wakeup from sleep count: " + String(wakeup_count));
  wakeup_count++;

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using EXT0 RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using EXT1 RTC_CNTL"); print_GPIO_wake_up(); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep, wakeup reason: %d\n",wakeup_reason); break;
  }

  return "This is the " + String(wakeup_count) + "th wakeup in a row, for reason (%d): " + String(wakeup_reason);
}

/*
Method to print the GPIO that triggered the wakeup
*/
void print_GPIO_wake_up(){
  uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
  Serial.print("GPIO pin(s) that woke it up: " + String(GPIO_reason) + " which translates to pin: ");
  Serial.println((log(GPIO_reason))/log(2), 0);
}

/*
 * The sleep duration of the piggy should depend on the battery voltage.
 * 0 means the piggy should not go to sleep
 *
 * >4.18V => nosleep (the heuristic to detect battery by measuring the voltage variance fails sometimes, but in that case, the voltage is 4.23V so device stays on)
 * >4.10V => wakeup every 1 hour
 * >4.00V => wakeup every 2 hours
 * >3.90V => wakeup every 4 hours
 * >3.80V => wakeup every 6 hours
 * >3.70V => wakeup every 8 hours
 * <3.70V => wakeup every 10 hours (should not happen because battery completely drained)
 */
int batteryVoltageToSleepSeconds(double voltage) {
  if (voltage > 4.18) {
    return 0; // don't go to sleep
  } else if (voltage > 4.10) {
    return 60*60;
  } else if (voltage > 4.0) {
    return 2*60*60;
  } else if (voltage > 3.9) {
    return 4*60*60;
  } else if (voltage > 3.8) {
    return 6*60*60;
  } else if (voltage > 3.7) {
    return 8*60*60;
  } else {
    return 10*60*60;
  }
}

// returns: true if hibernate was checked, false if it was rate limited
bool hibernateDependingOnBattery() {
  // rate limit these hibernate checks because it seems to become buggy 
  long nowCheckedHibernate = millis();
  if ((nowCheckedHibernate - lastHibernateCheck) < HIBERNATE_CHECK_PERIOD_MILLIS) {
    return false;
  } else {
    lastHibernateCheck = nowCheckedHibernate;
  }
  Serial.printf("Free heap memory: %" PRIu32 " bytes (hibernateDependingOnBattery)\r\n", ESP.getFreeHeap());

  int resetReason = rtc_get_reset_reason(0);
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  // if not payment came in, then lastPaymentReceivedMillis = 0, so it stays awake after boot if it was a manual wakeup
  if ((resetReason == POWERON_RESET || resetReason == SW_CPU_RESET || wakeup_reason == ESP_SLEEP_WAKEUP_EXT0 || wakeup_reason == ESP_SLEEP_WAKEUP_EXT1)
    && (millis()-lastPaymentReceivedMillis) < (AWAKE_SECONDS_AFTER_MANUAL_WAKEUP*1000)) {
    Serial.println("Device was woken up manually or it crashed or received payment less than " + String(AWAKE_SECONDS_AFTER_MANUAL_WAKEUP) + "s ago, not sleeping yet because another payment might come in..");
    return true;
  }

  double voltage = getBatteryVoltage(); // takes around 450ms (due to 4 battery voltage samples with 100ms delay between)
  // voltage < 0 means it's battery powered, but sometimes it can glitch and show NOT battery powered
  // to avoid going to sleep on just a glitch, don't go to sleep as long as the average battery voltage is not also negative.
  if (voltage < 0) {
    Serial.println("Device is not battery powered so not sleeping.");
    return true;
  } else if (getAverageBatteryVoltage()<2.5) {
    Serial.println("Saved from an unwanted sleep by the average battery voltage (" + String(getAverageBatteryVoltage()) + "<2.5V) check!");
    return true;
  }

  int sleepTimeSeconds = batteryVoltageToSleepSeconds(voltage);
  if (sleepTimeSeconds == 0) {
    Serial.println("Device has extremely full battery so not sleeping.");
    return true;
  } else {
    Serial.println("Battery voltage is " + String(voltage) + " so sleeping for " + String(sleepTimeSeconds) + "seconds.");
    hibernate(sleepTimeSeconds);
  }
  return true;
}

void hibernate(int sleepTimeSeconds) {
  Serial.println("Going to sleep for " + String(sleepTimeSeconds) + " seconds...");

  displayStatus(true);

  disconnectWebsocket();
  // Disconnect wifi cleanly because some access points will refuse reconnections if this is forgotten...
  disconnectWifi();

  uint64_t deepSleepTime = (uint64_t)sleepTimeSeconds * (uint64_t)1000 * (uint64_t)1000;
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON); // RTC peripherals needs to stay on for GPIO32's pulldown to work
  // disabled to allow for wakeup_count to stay: esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON); // make sure it's ON, otherwise the OFF will fail an assert
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_ON); // make sure it's ON, otherwise the OFF will fail an assert
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
  esp_sleep_enable_timer_wakeup(deepSleepTime);

  // Enable GPIO39 (user button) wakeup
  // GPIO39 is active low, which ext1 doesn't support so we need ext0:
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_39,0); //1 = High, 0 = Low

  // Enable GPIO32 (tilt sensor pin wakeup)
  // ext0 only supports a single wakeup pin so we need ext1 for an additional one:
  rtc_gpio_pullup_dis(GPIO_NUM_32);  // pullup disabled because pulldown enabled
  rtc_gpio_pulldown_en(GPIO_NUM_32); // pulldown is needed to avoid floating pin that triggers randomly
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);

  esp_deep_sleep_start();
}

void resetLastPaymentReceivedMillis() {
  lastPaymentReceivedMillis = millis();
}
