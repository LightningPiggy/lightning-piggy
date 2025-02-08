/**
 * Configured values can be set by:
 * - the new method: modifying the /config.json file in LittleFS (this is done using the web server on the device)
 * - the old method: replacing the REPLACETHISBY... values in the binary (this is done using the webinstaller in a webbrowser, while the device is connecting with a USB cable)
 */

#include "FS.h"
#include <LittleFS.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// index page:
const char* htmlContent PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Config Editor</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; text-align: center; }
        textarea { width: 90%; height: 200px; font-family: monospace; }
        button { margin-top: 10px; padding: 10px 20px; font-size: 16px; }
        #response { margin-top: 20px; color: green; }
    </style>
</head>
<body>

    <h2>Configuration Editor</h2>
    <p>Enter JSON Configuration:</p>
    <textarea id="jsonInput">Fetching /config.json - please wait...</textarea>
    <br>
    <button onclick="saveConfig()">Save Configuration</button>
    <p id="response"></p>

    <script>
        function loadConfig() {
            fetch("/config.json")
            .then(response => response.ok ? response.text() : Promise.reject("Failed to load config"))
            .then(data => document.getElementById("jsonInput").value = data)
            .catch(error => {
                document.getElementById("jsonInput").value = '{ "error": "Failed to load config" }';
                console.error("Error loading config:", error);
            });
        }

        function saveConfig() {
            let jsonData = document.getElementById("jsonInput").value;
            fetch("/saveconfig", {
                method: "PUT",
                headers: { "Content-Type": "application/json" },
                body: jsonData
            })
            .then(response => response.text())
            .then(data => document.getElementById("response").innerText = "Response: " + data)
            .catch(error => {
                document.getElementById("response").innerText = "Error: " + error;
                console.error("Error:", error);
            });
        }

        window.onload = loadConfig;
    </script>

</body>
</html>
)rawliteral";

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

  server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", paramFileString);
  });

  server.on("/config.json", HTTP_PUT, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", "PUT received OK");
  });

  server.on("/saveconfig", HTTP_PUT, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "saveconfig OK");
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      static String body;  // Accumulate data chunks

      if (index == 0) body = "";  // Reset on new request

      body += String((char*)data).substring(0, len);  // Append chunk

      if (index + len == total) {  // All data received
          Serial.println("Received PUT data: '" + body + "'");
          paramFileString = body;
          writeFile("/config.json", body);
      }
  });

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
  });

}

void start_webserver() {
  Serial.println("Starting webserver!");
  Serial.printf("Before, free heap: %" PRIu32 "\n", ESP.getFreeHeap());
  server.begin();
  Serial.printf("After, free heap: %" PRIu32 "\n", ESP.getFreeHeap());
}
