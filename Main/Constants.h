#ifndef CONSTANTS_H
#define CONSTANTS_H

String currentVersion = "5.0.4";

/**
 * The piggy can be in different 'modes':
 * ================
 */

#define PIGGYMODE_INIT 0             // no wifi connection has been attempted => if no config then go to starting-ap, otherwise go to starting-sta
#define PIGGYMODE_SLEEP_BOOTSLOGAN 6 // waiting for bootslogan or logo to be seen
#define PIGGYMODE_STARTING_STA 1     // start wifi connection to station
#define PIGGYMODE_WAITING_STA 7      // wait for wifi connection to station
#define PIGGYMODE_STARTED_STA 2      // websocket handling and triggering refresh if it's time
#define PIGGYMODE_STARTED_STA_REFRESH_RECEIVECODE 8
#define PIGGYMODE_STARTED_STA_REFRESH_STATUS 9
#define PIGGYMODE_STARTED_STA_REFRESH_BALANCE_PAYMENTS 10
#define PIGGYMODE_STARTED_STA_CHECKUPDATE 11
#define PIGGYMODE_FAILED_STA 3       // failed to connect to station => show warning and how to trigger config mode and go to sleep
#define PIGGYMODE_STARTING_AP 4      // attempt to start AP + webserver and then go to started-ap
#define PIGGYMODE_STARTED_AP 5       // AP is up-and-running: wait until user trigger /reboot

// Configuration through access point:
#define ACCESS_POINT_SSID          "LightningPiggy Configuration"
#define ACCESS_POINT_PASS          ""
#define ACCESS_POINT_CHANNEL       1
#define ACCESS_POINT_MAX_STA_CONN  4

#define WEBCONFIG_USERNAME "piggy"
#define WEBCONFIG_PASSWORD "oinkoink"

#define CONFIG_FILE "/config.json"

extern const int NOT_SPECIFIED = -1; 

const char * NOTCONFIGURED = "REPLACETHISBY";
const unsigned int NOTCONFIGURED_LENGTH = 13;
#define MAX_CONFIG_LENGTH 131

// Maximum time to show the bootslogan
extern const int MAX_BOOTSLOGAN_SECONDS = 15;

#define MAX_PAYMENTS 6 // even the 2.66 inch display can only fit 6

extern const int MAX_WATCHDOG_REBOOTS = 3;
extern const int SLEEP_HOURS_AFTER_MAX_WATCHDOG_REBOOTS = 6;

#define AWAKE_SECONDS_AFTER_MANUAL_WAKEUP 3*60
#define AWAKE_SECONDS_AS_ACCESS_POINT 5*60

#define CHECK_UPDATE_PERIOD_SECONDS 7*24*60*60 // every week

#define UPDATE_BALANCE_PERIOD_MILLIS 1000 * 60 * 15 // fallback to updating balance every 15 minutes if the instant websocket method is unavailable

#define UPDATE_VOLTAGE_PERIOD_MILLIS 1000 * 60 * 5 // update voltage display every 5 minutes

#define HIBERNATE_CHECK_PERIOD_MILLIS 1000 * 30 // hibernate check every 30 seconds

#define TIME_BETWEEN_WEBSOCKET_CONNECTION_ATTEMPTS 1000 * 30

#define WIFI_CONNECT_TIMEOUT_SECONDS 30         // after this time, it's deemed a failure

#define DEFAULT_LNBITS_PORT 443

#define HTTPS_TIMEOUT_SECONDS 15
#define MAX_HTTPS_CONNECTION_ATTEMPTS 3

// In alphabetical order
const char * deWeekdays[] = { "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
const char * dkWeekdays[] = { "Sø", "Ma", "Ti", "On", "To", "Fr", "Lø"};
const char * esWeekdays[] = { "Do", "Lu", "Ma", "Mi", "Ju", "Vi", "Sá"};
const char * enWeekdays[] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
const char * nlWeekdays[] = { "Zo", "Ma", "Di", "Wo", "Do", "Vr", "Za"};


typedef enum {
    STR2INT_SUCCESS,
    STR2INT_OVERFLOW,
    STR2INT_UNDERFLOW,
    STR2INT_INCONVERTIBLE
} str2int_errno;

String websocketApiUrl = "/api/v1/ws/";

#define DISPLAY_TYPE_213DEPG 1
#define DISPLAY_TYPE_266DEPG 2

const char* defaultTimeServer = "www.timeapi.io";
const char* defaultTimeServerPath = "/api/time/current/zone?timeZone=";

/* No longer works:
const char* defaultTimeServer = "worldtimeapi.org";
const char* defaultTimeServerPath = "/api/timezone/";
*/

const char* defaultCheckUpdateHost = "m.lightningpiggy.com";

const char* defaultThousandsSeparator = ",";
const char* defaultDecimalSeparator = ".";

#define smallestFontHeight 16
#define MAX_FONT 5

#define MAX_TEXT_LINES 20

#endif // #ifndef CONSTANTS_H
