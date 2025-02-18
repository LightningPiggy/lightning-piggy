/**
 * Configured values can be set by:
 * - the new method: modifying the CONFIG_FILE in LittleFS (this is done using the web server on the device)
 * - the old method: replacing the REPLACETHISBY... values in the binary (this is done using the webinstaller in a webbrowser, while the device is connecting with a USB cable)
 */

#include "FS.h"
#include <LittleFS.h>

#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

// index page:
#include "../webconfig/index.html.gzip.h"

String paramFileString = "";

IPAddress apIP(192, 168, 4, 1);  // Hardcoded IP for the ESP32 SoftAP

DNSServer dnsServer;
AsyncWebServer server(80);
AsyncAuthenticationMiddleware digestAuth;


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

bool deleteFile(const char* filename) {
  Serial.printf("Deleting file: '%s' ", filename);
  if (LittleFS.remove(filename)) {
    Serial.println("- OK");
    return true;
  }
  Serial.println("- FAILED!");
  return false;
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

void tryGetJsonValue(JsonDocument &doc, const char *key, char **output, size_t maxLength, char* binaryReplacedValue, const char * defaultValue = NULL) {
    String value = getJsonValue(doc, key);
    if (value == "") {
      Serial.println("WARNING: no Json config value found for '" + String(key) + "' so checking for binaryReplacedValue and defaultValue...");
      if (isConfigured(binaryReplacedValue)) {
        Serial.println("Found binaryReplacedValue, using that: '" + String(binaryReplacedValue) + "'");
        value = binaryReplacedValue;
      } else if (defaultValue != NULL) {
        value = defaultValue;
      } else {
        Serial.println("no binaryReplacedValue is set, no defaultValue is set, so config item remains empty");
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
        Serial.println("Still continuing because binaryReplacedValue or defaultValue might quality.");
    }

    // Mandatory:
    // ==========
    tryGetJsonValue(doc, "config_wifi_ssid_1", &ssid, MAX_CONFIG_LENGTH, REPLACE_ssid);
    tryGetJsonValue(doc, "config_wifi_password_1", &password, MAX_CONFIG_LENGTH, REPLACE_password);
    tryGetJsonValue(doc, "config_lnbits_host", &lnbitsHost, MAX_CONFIG_LENGTH, REPLACE_lnbitsHost);
    tryGetJsonValue(doc, "config_lnbits_invoice_key", &lnbitsInvoiceKey, MAX_CONFIG_LENGTH, REPLACE_lnbitsInvoiceKey);

    Serial.println("Parsed config:");
    Serial.printf("config_wifi_ssid_1: %s\n", ssid);
    Serial.printf("config_wifi_password_1: %s\n", password);
    Serial.printf("config_lnbits_host: %s\n", lnbitsHost);
    Serial.printf("config_lnbits_invoice_key: %s\n", lnbitsInvoiceKey);

    // Optional
    // ========

    // Wallet:
    tryGetJsonValue(doc, "config_lnbits_https_port", &lnbitsPort, MAX_CONFIG_LENGTH, REPLACE_lnbitsPort);
    tryGetJsonValue(doc, "config_static_receive_code", &staticLNURLp, MAX_CONFIG_LENGTH, REPLACE_staticLNURLp);

    // Display:
    tryGetJsonValue(doc, "config_fiat_currency", &btcPriceCurrencyChar, MAX_CONFIG_LENGTH, REPLACE_btcPriceCurrencyChar);
    tryGetJsonValue(doc, "config_balance_bias", &balanceBias, MAX_CONFIG_LENGTH, REPLACE_balanceBias);
    tryGetJsonValue(doc, "config_thousands_separator", &thousandsSeparator, MAX_CONFIG_LENGTH, REPLACE_thousandsSeparator, defaultThousandsSeparator);
    tryGetJsonValue(doc, "config_decimal_separator", &decimalSeparator, MAX_CONFIG_LENGTH, REPLACE_decimalSeparator, defaultDecimalSeparator);
    tryGetJsonValue(doc, "config_boot_salutation", &bootSloganPrelude, MAX_CONFIG_LENGTH, REPLACE_bootSloganPrelude);
    tryGetJsonValue(doc, "config_show_boot_wisdom", &showSloganAtBoot, MAX_CONFIG_LENGTH, REPLACE_showSloganAtBoot, "NO");

    // Device:
    tryGetJsonValue(doc, "config_locale", &localeSetting, MAX_CONFIG_LENGTH, REPLACE_localeSetting);
    tryGetJsonValue(doc, "config_time_zone", &timezone, MAX_CONFIG_LENGTH, REPLACE_timezone);

    // Advanced:
    tryGetJsonValue(doc, "config_always_run_webserver", &alwaysRunWebserver, MAX_CONFIG_LENGTH, REPLACE_alwaysRunWebserver, "NO");
    tryGetJsonValue(doc, "config_update_host", &checkUpdateHost, MAX_CONFIG_LENGTH, REPLACE_updateHost, defaultCheckUpdateHost);
    tryGetJsonValue(doc, "config_time_server", &timeServer, MAX_CONFIG_LENGTH, REPLACE_timeServer, defaultTimeServer);
    tryGetJsonValue(doc, "config_time_server_path", &timeServerPath, MAX_CONFIG_LENGTH, REPLACE_timeServerPath, defaultTimeServerPath);

    Serial.printf("config_lnbits_https_port: %s\n", lnbitsPort);

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

  paramFileString = readFile(CONFIG_FILE);
  parseConfig(paramFileString);

  Serial.println("LittleFS setup done.");
}

// Returns true if the value is configured, otherwise false.
bool isConfigured(const char * configValue) {
  if (configValue == NULL || strnlen(configValue, MAX_CONFIG_LENGTH) == 0 || strncmp(configValue, NOTCONFIGURED, NOTCONFIGURED_LENGTH) == 0) {
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
  Serial.println("Setting up webserver...");

  digestAuth.setUsername(WEBCONFIG_USERNAME);
  digestAuth.setPassword(WEBCONFIG_PASSWORD);
  digestAuth.setRealm(ACCESS_POINT_SSID);
  digestAuth.setAuthFailureMessage("Authentication failed");
  digestAuth.setAuthType(AsyncAuthType::AUTH_DIGEST);
  digestAuth.generateHash();
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_gzip, index_gzip_length);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  }).addMiddleware(&digestAuth);

  server.on(CONFIG_FILE, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", paramFileString);
  }).addMiddleware(&digestAuth);

  server.on("/connect-wifi-station", HTTP_GET, [](AsyncWebServerRequest* request) {
    //request->send(200, "text/html", "Trying to connect to WiFi station. If this fails, the device will go back into Access Point mode.");
    piggyMode = PIGGYMODE_STARTING_STA;
  }).addMiddleware(&digestAuth);

  server.on("/restart", HTTP_POST, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", "Restarting the device...");
    disconnectWifi();
    Serial.println("Restarting...");
    ESP.restart();
  }).addMiddleware(&digestAuth);

  server.on("/delete-config", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (deleteFile(CONFIG_FILE)) {
      request->send(200, "text/html", "Configuration file deleted.");
    } else {
      request->send(200, "text/html", "WARNING: Failed to delete configuration file!");
    }
  }).addMiddleware(&digestAuth);

  server.on(CONFIG_FILE, HTTP_PUT, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "Configuration file saved.");
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      static String body;  // Accumulate data chunks

      if (index == 0) body = "";  // Reset on new request

      body += String((char*)data).substring(0, len);  // Append chunk

      if (index + len == total) {  // All data received
          Serial.println("Received PUT data: '" + body + "'");
          paramFileString = body;
          writeFile(CONFIG_FILE, body);
          parseConfig(paramFileString);
      }
  }).addMiddleware(&digestAuth);

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
  });

}

void start_webserver() {
  Serial.println("Starting webserver...");
  Serial.printf("Before, free heap: %" PRIu32 "\n", ESP.getFreeHeap());
  server.begin();
  Serial.printf("After, free heap: %" PRIu32 "\n", ESP.getFreeHeap());
}

void start_dns() {
  Serial.println("Starting captive DNS server...");
  dnsServer.start(53, "*", apIP);
}

void loop_dns() {
  dnsServer.processNextRequest();  // Handle DNS requests
}

void stop_webserver() {
  server.end();
}

// Check if enough items are configured to attempt regular startup.
bool hasMinimalConfig() {
  Serial.println("Checking for minimal configuration...");
  if (isConfigured(ssid) && isConfigured(lnbitsHost) && isConfigured(lnbitsInvoiceKey)) {
    Serial.println("All mandatory configuration items are set.");
    return true;
  }
  Serial.println("WARNING: missing mandatory configuration items!");
  return false;
}
