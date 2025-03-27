#include "../timezones/timezones.h"

void setup_time() {
  Serial.println("Initializing time using NTP...");
  nostr::esp32::ESP32Platform::initTime("pool.ntp.org");

  Serial.println("UTC time: " + getLocalTimeAsString());

  if (isConfigured(timezone)) {
    Serial.println("Setting local timezone...");
    // Later on, this could validate the timezone even more,
    // and show a clear error if there's something wrong with it.
    setTimeZone(timezone);
    Serial.print("Local time after setTimeZone: " + getLocalTimeAsString());
  }
}

String getTimeFromNTP() {
  #ifdef DEBUG
  lastTime = "W23:39";
  return "W23:39";
  #endif
  return getDayOfWeekString(getDayOfWeek()) + getLocalTimeAsString();
}

// In alphabetical order
String getDayOfWeekString(int dayOfWeek) {
  if (dayOfWeek < 0 || dayOfWeek > 6) {
    Serial.println("Invalid day of week: " + String(dayOfWeek));
    return "";
  }
  if (strncmp(localeSetting,"de",2) == 0) {
     return deWeekdays[dayOfWeek];
  } else if (strncmp(localeSetting,"dk",2) == 0) {
     return dkWeekdays[dayOfWeek];
  } else if (strncmp(localeSetting,"nl",2) == 0) {
    return nlWeekdays[dayOfWeek];
  } else if (strncmp(localeSetting,"es",2) == 0) {
    return esWeekdays[dayOfWeek];
  } else {
     return enWeekdays[dayOfWeek];
  }
}

// Function to find POSIX TZ string in PROGMEM
const char* getPosixTZ(const char* city) {
    for (int i = 0; i < TIMEZONE_COUNT; i++) {
        char cityBuffer[50], tzBuffer[50];
        strcpy_P(cityBuffer, (PGM_P)pgm_read_ptr(&(timeZoneMap[i].city)));
        strcpy_P(tzBuffer, (PGM_P)pgm_read_ptr(&(timeZoneMap[i].posixTZ)));

        if (strcmp(cityBuffer, city) == 0) {
            return strdup(tzBuffer);  // Return a copy since PROGMEM is read-only
        }
    }
    return "GMT0"; // Default fallback
}

void setTimeZone(const char* city) {
    const char* posixTZ = getPosixTZ(city);
    Serial.printf("Setting Timezone: %s -> %s\n", city, posixTZ);
    setenv("TZ", posixTZ, 1);
    tzset();
}

String getLocalTimeAsString() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "Failed to obtain time";
    }
    char timeString[64];  // Buffer to hold the formatted time string
    strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);
    return String(timeString);
}

int getDayOfWeek() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return -1;  // Return -1 to indicate error
    }
    return timeinfo.tm_wday;  // tm_wday is 0-6 (Sunday = 0, Saturday = 6)
}
