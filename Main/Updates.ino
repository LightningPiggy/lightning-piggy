#include "Constants.h"

String newVersion = ""; // used by the update checker

int lastChecked = NOT_SPECIFIED;

// The server will only return a new version if the current version is LOWER THAN the current version.
// Otherwise, the server will just return the current version that the client sent in the User-Agent string.
bool isUpdateAvailable() {
  newVersion.trim();
  currentVersion.trim();
  Serial.println("currentVersion: '" + currentVersion + "' of length: " + String(currentVersion.length()));
  Serial.println("newVersion: '" + newVersion + "' of length: " + String(newVersion.length()));
  if (newVersion != "") {
    Serial.println("newVersion is not empty, this is good");
    if (newVersion != currentVersion) {
      Serial.println("newVersion != currentVersion, this is good");
      return true;
    } else {
      Serial.println("newVersion == currentVersion, no update");
    }
  } else {
    Serial.println("newVersion is empty");
  }
  return false;
}

void checkShowUpdateAvailable() {
  if (lastChecked == NOT_SPECIFIED || (millis()-lastChecked > CHECK_UPDATE_PERIOD_SECONDS*1000)) {
    newVersion = checkNewVersion();
    Serial.println("checkNewVersion returned: '" + newVersion + "' of length: " + String(newVersion.length()));
    if (newVersion != "") {
      lastChecked = millis();
      if (isUpdateAvailable()) {
        Serial.println("Update available!");
        displayStatus(false);
      } else {
        Serial.println("No update available.");
      }
    } else {
      Serial.println("checkNewVersion() returned '' so could not check for updates");
    }
  } else {
    //Serial.println("Already checked for updates recently, not doing it again.");
  }
}

String getShortHardwareInfo() {
  int displayToUse = getDisplayToUse();
  if (displayToUse == DISPLAY_TYPE_213DEPG) {
    return "2.13D";
  } else if (displayToUse == DISPLAY_TYPE_266DEPG) {
    return "2.66D";
  } else {
    return "UNKNOWN";
  }
}

String getLongHardwareInfo() {
  // Since the unified builds, we only know which display it is, not really which hardware.
  // But to be backwards compatible with the update checker metrics, we assume
  // a 2.13DEPG display must be the LILYGOT5V213
  // and 2.66DEPG must be the LILYGOT5V266
  int displayToUse = getDisplayToUse();
  if (displayToUse == DISPLAY_TYPE_213DEPG) {
    return "LILYGOT5V213|DEPG0213BN";
  } else if (displayToUse == DISPLAY_TYPE_266DEPG) {
    return "LILYGOT5V266|DEPG0266BN";
  } else {
    return "UNKNOWN|UNKNOWN";
  }
}

String getShortVersion() {
  return currentVersion;
}

String getFullVersion() {
  const char compiletime[] = __DATE__ " " __TIME__;
  String compileTime(compiletime);
  return currentVersion + "|" + getLongHardwareInfo() + "|" + compileTime;
}

/*
 * Check if there's a new version available for the current version and hardware combination.
 *
 * The server decides whether or not the new version should be installed, which enables more advanced
 * things like incremental rollouts, or holding back an update if there are issues in the field.
 *
 * returns: the new version that should be installed
 */
String checkNewVersion() {
  Serial.print("Checking for updates: ");
  return getEndpointData(checkUpdateHost, "/", false);
}
