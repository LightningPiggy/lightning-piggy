#include "FS.h"
#include <LittleFS.h>

String staticConfig = R"(
[
  {
    "name": "config_wifi_ssid_1",
    "value": "New Wifi",
    "label": "WiFi SSID",
    "type": "text"
  },
  {
    "name": "config_wifi_password_1",
    "value": "",
    "label": "WiFi Password",
    "type": "text"
  }
]
)";

String readFile(String name) {
    File file = LittleFS.open(name, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return "";
    }

    String fileContent = file.readString();

    Serial.println("File Content:");
    Serial.println(fileContent);

    file.close();
}

bool writeFile(const char* filename, const String& content) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        Serial.printf("Failed to open file: %s for writing\n", filename);
        return false;
    }

    if (file.print(content)) {
        Serial.printf("File written successfully: %s\n", filename);
        file.close();
        return true;
    } else {
        Serial.printf("Failed to write to file: %s\n", filename);
        file.close();
        return false;
    }
}

void setup_config() {

  Serial.println("Attempting to mount LittleFS filesystem...");
  if (!LittleFS.begin(false)) {
    Serial.println("LittleFS mount failed, formatting...");
    if (!LittleFS.format()) {
      Serial.println("LittleFS format failed, can't read or use config file! This is bad, should be a warning on-screen!");
      return;
    } else {
      Serial.println("Format successfull, mounting...");
      if (!LittleFS.begin(false)) {
        Serial.println("LittleFS mount STILL failed, even after successful formatting! This is bad, should be a warning on-screen!");
        return;
      }
    }
  }
  Serial.println("LittleFS mounted!");
  
  Serial.println(readFile("/config.json"));

  writeFile("/config_new.json", staticConfig);
  Serial.println(readFile("/config_new.json"));

  Serial.println("LittleFS setup done.");
}
