/**
 * Configured values can be set by:
 * - the new method: modifying the /config.json file in LittleFS (this is done using the web server on the device)
 * - the old method: replacing the REPLACETHISBY... values in the binary (this is done using the webinstaller in a webbrowser, while the device is connecting with a USB cable)
 */

#include "FS.h"
#include <LittleFS.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* PARAM_MESSAGE PROGMEM = "message";

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

const char* htmlContent PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Sample HTML</title>
</head>
<body>
    <h1>Hello, World!</h1>
    <p>This is HTML.</p>
</body>
</html>
)";

String paramFileString = "";

AsyncWebServer server(80);


String readFile(String name) {
    File file = LittleFS.open(name, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return "";
    }

    String fileContent = file.readString();

    Serial.println("File Content:" + fileContent);

    file.close();

    return fileContent;
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

String getJsonValue(JsonDocument &doc, const char *name) {
  for (JsonObject elem : doc.as<JsonArray>()) {
    if (strcmp(elem["name"], name) == 0) {
      String value = elem["value"].as<String>();
      Serial.println("Found config value: '" + value + "'");
      return value;
    }
  }
  return "";
}

void tryGetJsonValue(JsonDocument &doc, const char *key, char **output, size_t maxLength, char* binaryReplacedValue) {
    String value = getJsonValue(doc, key);
    if (value == "") {
      Serial.println("WARNING: no Json config value found for '" + String(key) + "' so checking for binaryReplacedValue...");
      if (isConfigured(binaryReplacedValue)) {
        Serial.println("Found binaryReplacedValue, using that: '" + String(binaryReplacedValue) + "'");
        value = binaryReplacedValue;
      } else {
        Serial.println("no binaryReplacedValue is set, config value is empty");
      }
    }
    free(*output);  // Free old memory to prevent leaks
    *output = strndup(value.c_str(), maxLength - 1);  // Allocate new memory and duplicate into it
}

bool parseConfig(String paramFileString) {
    StaticJsonDocument<6000> doc;
    DeserializationError error = deserializeJson(doc, paramFileString);
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return false;
    }

    tryGetJsonValue(doc, "config_wifi_ssid_1", &ssid, MAX_CONFIG_LENGTH, REPLACE_ssid);
    tryGetJsonValue(doc, "config_wifi_password_1", &password, MAX_CONFIG_LENGTH, REPLACE_password);

    Serial.printf("SSID: %s\n", ssid);
    Serial.printf("Password: %s\n", password);

    return true;
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

  paramFileString = readFile("/config.json");
  parseConfig(paramFileString);

  //writeFile("/config_new.json", staticConfig); Serial.println(readFile("/config_new.json"));

  Serial.println("LittleFS setup done.");
}

// Returns true if the value is configured, otherwise false.
bool isConfigured(const char * configValue) {
  if ((strncmp(configValue, NOTCONFIGURED, NOTCONFIGURED_LENGTH) == 0) || (strlen(configValue) == 0)) {
    return false;
  } else {
    return true;
  }
}

int getConfigValueAsInt(char* configValue, int defaultValue) {
  int configInt = defaultValue;
  if (isConfigured(configValue)) {
    if (str2int(&configInt, (char*)configValue, 10) != STR2INT_SUCCESS) {
      Serial.println("WARNING: failed to convert config value ('" + String(configValue) + "') to integer, ignoring...");
    } else {
      Serial.println("Returning config value as int: " + String(configInt));
    }
  }
  return configInt;
}

void setup_webserver() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", htmlContent);
  });

  // Send a GET request to <IP>/get?message=<message>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest* request) {
    String message;
    if (request->hasParam(PARAM_MESSAGE)) {
      message = request->getParam(PARAM_MESSAGE)->value();
    } else {
      message = "No message sent";
    }
    request->send(200, "text/plain", "Hello, GET: " + message);
  });

  // Send a POST request to <IP>/post with a form field message set to <message>
  server.on("/post", HTTP_POST, [](AsyncWebServerRequest* request) {
    String message;
    if (request->hasParam(PARAM_MESSAGE, true)) {
      message = request->getParam(PARAM_MESSAGE, true)->value();
    } else {
      message = "No message sent";
    }
    request->send(200, "text/plain", "Hello, POST: " + message);
  });

   // catch any request, and send a 404 Not Found response
  // except for /game_log which is handled by onRequestBody
  server.onNotFound([](AsyncWebServerRequest* request) {
    if (request->url() == "/game_log")
      return; // response object already creted by onRequestBody

    request->send(404, "text/plain", "Not found");
  });
}

void start_webserver() {
  Serial.println("Starting webserver!");
  Serial.printf("Before, free heap: %" PRIu32 "\n", ESP.getFreeHeap());
  server.begin();
  Serial.printf("After, free heap: %" PRIu32 "\n", ESP.getFreeHeap());
}
