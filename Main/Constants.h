#ifndef CONSTANTS_H
#define CONSTANTS_H

String currentVersion = "6.3.0";

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

#define PIGGYMODE_STARTED_STA_REFRESH_BALANCE 10
#define PIGGYMODE_STARTED_STA_WAIT_BALANCE 13
#define PIGGYMODE_STARTED_STA_RECEIVED_BALANCE 14

#define PIGGYMODE_STARTED_STA_REFRESH_PAYMENTS 15
#define PIGGYMODE_STARTED_STA_WAIT_PAYMENTS 16
#define PIGGYMODE_STARTED_STA_RECEIVED_PAYMENTS 17

#define PIGGYMODE_STARTED_STA_CHECKUPDATE 11
#define PIGGYMODE_FAILED_STA 3       // failed to connect to station => show warning and how to trigger config mode and go to sleep
#define PIGGYMODE_STARTING_AP 4      // attempt to start AP + webserver and then go to started-ap
#define PIGGYMODE_STARTED_AP 5       // AP is up-and-running: wait until user trigger /reboot

#define PIGGYMODE_RECONNECT_WIFI 12

// Configuration through access point:
#define ACCESS_POINT_SSID          "Piggy Config"
#define ACCESS_POINT_PASS          ""
#define ACCESS_POINT_CHANNEL       1
#define ACCESS_POINT_MAX_STA_CONN  4

#define WEBCONFIG_USERNAME "piggy"
#define WEBCONFIG_PASSWORD "oinkoink"

#define CONFIG_FILE "/config.json"

extern const int NOT_SPECIFIED = -1; 

#define MAX_CONFIG_LENGTH 256 // used to be 131 but people with longer LNURLs had issues
#define MAX_CONFIG_LENGTH_NWCURL 512

// Maximum time to show the bootslogan
extern const int MAX_BOOTSLOGAN_SECONDS = 15;

#define MAX_PAYMENTS 5 // even the 2.66 inch display can only fit 6

extern const int MAX_WATCHDOG_REBOOTS = 3;
extern const int SLEEP_HOURS_AFTER_MAX_WATCHDOG_REBOOTS = 6;

#define AWAKE_SECONDS_AFTER_MANUAL_WAKEUP 3*60
#define AWAKE_SECONDS_AS_ACCESS_POINT 5*60

#define CHECK_UPDATE_PERIOD_SECONDS 24*60*60 // every 24 hours

#define FETCH_FIAT_BALANCE_PERIOD_MS 15*60*1000 // every 15 minutes

#define NWC_UPDATE_BALANCE_PERIOD_MILLIS 1000 * 60 * 5 // NWC has notifications so these manual updates are only a fallback
#define LNBITS_UPDATE_BALANCE_PERIOD_MILLIS 1000 * 60 * 15 // LNBits should have websockets so these manual updates are only a fallback

#define HIBERNATE_CHECK_PERIOD_MILLIS 1000 * 10 // hibernate check every 10 seconds

#define TIME_BETWEEN_WEBSOCKET_CONNECTION_ATTEMPTS 1000 * 30

#define WIFI_CONNECT_TIMEOUT_SECONDS 30         // after this time, it's deemed a failure

#define DEFAULT_SLEEP_DURATION_SECONDS 60 * 60 * 6          // 6 hours = morning, afternoon, evening and night

#define PERIODIC_RESTART_MILLIS 1000 * 60 * 60 * 23    // restart every 23 hours to handle any long-duration issues such as out of memory

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
#define DISPLAY_TYPE_213DEPG_QEMU 3

#define DISPLAY_HEIGHT_213DEPG 250
#define DISPLAY_HEIGHT_266DEPG 296

#define DISPLAY_WIDTH_213DEPG 122
#define DISPLAY_WIDTH_266DEPG 152

const char* defaultCheckUpdateHost = "m.lightningpiggy.com";

const char* defaultThousandsSeparator = ",";
const char* defaultDecimalSeparator = ".";

#define smallestFontHeight 16
#define MAX_FONT 5

#define MAX_TEXT_LINES 10

#define WALLET_NONE 0
#define WALLET_LNBITS 1
#define WALLET_NWC 2

String configURL = "http://192.168.4.1/";
String loginURL = "/login";

static const char login_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<body style="background-color: #F0BCD7; font-family: Arial, sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0;">
<div style="background: white; border-radius: 0.75rem; box-shadow: 0 4px 16px rgba(0, 0, 0, 0.05); padding: 2rem; width: 300px; text-align: center;">
<h2 style="color: #EC008C; margin-bottom: 1.5rem;">Lightning Piggy Configuration Login</h2>
<div id="error-message" style="color: #EC008C; margin-bottom: 1rem; display: none;">Invalid credentials, please try again.</div>
<form action="/submit" method="POST">
<input type="text" name="username" placeholder="Username" style="width: 100%; padding: 0.5rem; margin-bottom: 1rem; border: 1px solid #ccc; border-radius: 4px; transition: all 0.3s ease;" onfocus="this.style.borderColor='#EC008C'; this.style.boxShadow='0 0 0 2px rgba(236, 0, 140, 0.2)';" onblur="this.style.borderColor='#ccc'; this.style.boxShadow='none';">
<input type="password" name="password" placeholder="Password" style="width: 100%; padding: 0.5rem; margin-bottom: 1.5rem; border: 1px solid #ccc; border-radius: 4px; transition: all 0.3s ease;" onfocus="this.style.borderColor='#EC008C'; this.style.boxShadow='0 0 0 2px rgba(236, 0, 140, 0.2)';" onblur="this.style.borderColor='#ccc'; this.style.boxShadow='none';">
<input type="submit" value="Login" style="width: 100%; padding: 0.75rem; background-color: silver; color: black; border: none; border-radius: 4px; cursor: pointer; transition: background 0.3s, transform 0.3s;" onmouseover="this.style.backgroundColor='#EC008C'; this.style.color='white';" onmouseout="this.style.backgroundColor='silver'; this.style.color='black';">
</form>
<script>
if (window.location.search.includes('invalid')) {
  document.getElementById('error-message').style.display = 'block';
}
</script>
</div>
</body>
</html>
)rawliteral";

#endif // #ifndef CONSTANTS_H
