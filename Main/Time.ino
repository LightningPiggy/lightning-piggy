#include "../timezones/timezones.h"

String lastTime = "";

String getTimeFromNTP() {
  // Later on, this could validate the timezone even more,
  // and show a clear error if there's something wrong with it.
  if (!isConfigured(timezone)) return "";
  if (!wifiConnected()) return "NoNetwork";

  #ifdef DEBUG
  lastTime = "W23:39";
  return "W23:39";
  #endif
  String timeData = getEndpointData(timeServer, String(timeServerPath) + String(timezone), false);
  Serial.println("Got timeData: " + timeData);

  DynamicJsonDocument doc(8192); 

  DeserializationError error = deserializeJson(doc, timeData);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return "time error";
  }

  Serial.println("Extracting weekday and time from received data");

  /* worldtimeapi.org
  String datetimeAsString = doc["datetime"];
  int dayOfWeek = doc["day_of_week"];
  String dayOfWeekAsString = getDayOfWeekString(dayOfWeek);
  String timeString = datetimeAsString.substring(datetimeAsString.indexOf("T") + 1, datetimeAsString.indexOf("T") + 6); // Extract only the time (hh:mm)
  */

  /* timeapi.io response:
   *  {
        "year": 2025,
        "month": 2,
        "day": 20,
        "hour": 9,
        "minute": 9,
        "seconds": 58,
        "milliSeconds": 105,
        "dateTime": "2025-02-20T09:09:58.1052938",
        "date": "02/20/2025",
        "time": "09:09",
        "timeZone": "Europe/Amsterdam",
        "dayOfWeek": "Thursday",
        "dstActive": false
      }
   */
  String timeString = doc["time"];
  String dayOfWeek = doc["dayOfWeek"];
  String dayOfWeekAsString = dayOfWeek.substring(0,2); // would be nice to translate it

  lastTime = dayOfWeekAsString + " " + timeString;
  return lastTime;
}

String getLastTime() {
  return lastTime;
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

void printLocalTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}
