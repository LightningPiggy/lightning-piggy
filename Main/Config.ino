#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <WiFiManager.h>

bool shouldSaveConfig = false;

char lnbitsHost[60];
char lnbitsPort[6];
char invoiceKey[32];

void saveConfigCallback()
{
  shouldSaveConfig = true;
}

void resetConfig()
{
  Serial.println("formatting FS");
  SPIFFS.format();
  ESP.restart();
}

bool loadConfigFromFS()
{
  // read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin())
  {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json"))
    {
      // file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        JsonDocument json;
        auto deserializeError = deserializeJson(json, buf.get());
        if (!deserializeError)
        {
          Serial.println("Parsed JSON Config");
          serializeJsonPretty(json, Serial);

          // verify that keys exist in the json
          if (json["lnbitsHost"].isNull() || json["lnbitsPort"].isNull() || json["invoiceKey"].isNull())
          {
            Serial.println("JSON Config is missing keys, resetting to defaults");
            resetConfig();
            return false;
          }

          strcpy(lnbitsHost, json["lnbitsHost"]);
          strcpy(lnbitsPort, json["lnbitsPort"]);
          strcpy(invoiceKey, json["invoiceKey"]);
        }
        else
        {
          Serial.println("failed to load json config");
          Serial.println(deserializeError.c_str());
        }
        configFile.close();
      }
    }
    else
    {
      return false;
    }
  }
  else
  {
    Serial.println("failed to mount FS");
    resetConfig();
    return false;
  }

  Serial.println("Loaded configs from FS");

#ifdef DEBUG
  Serial.print("LNbits Host: ");
  Serial.println(lnbitsHost);

  Serial.print("LNbits Port: ");
  Serial.println(lnbitsPort);

  Serial.print("Invoice Key: ");
  Serial.println(invoiceKey);
#endif

  return true;
}

bool saveConfigToDisk()
{
  Serial.println("saving config");
  JsonDocument json;

  json["lnbitsHost"] = lnbitsHost;
  json["lnbitsPort"] = lnbitsPort;
  json["invoiceKey"] = invoiceKey;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile)
  {
    Serial.println("failed to open config file for writing");
    return false;
  }

  serializeJsonPretty(json, Serial);
  serializeJson(json, configFile);

  configFile.close();
  Serial.println("saved config");

  return true;
}

WiFiManagerParameter customParamLnBitsHost("lnbitsHost", "LN Bits Host", lnbitsHost, 60);
WiFiManagerParameter customParamLnBitsPort("lnbitsPort", "LN Bits Port", lnbitsPort, 6);
WiFiManagerParameter customParamInvoiceKey("invoiceKey", "Invoice Key", invoiceKey, 32);
bool setupConfigFromApServer()
{
  connectOrStartConfigAp(false);

  strcpy(lnbitsHost, customParamLnBitsHost.getValue());
  strcpy(lnbitsPort, customParamLnBitsPort.getValue());
  strcpy(invoiceKey, customParamInvoiceKey.getValue());

#ifdef DEBUG
  Serial.println("LN Bits Host: " + String(lnbitsHost));
  Serial.println("LN Bits Port: " + String(lnbitsPort));
  Serial.println("Invoice Key: " + String(invoiceKey));
#endif

  if (shouldSaveConfig)
  {
    return saveConfigToDisk();
  }

  return true;
}

bool loadConfigOrSetup()
{
  return loadConfigFromFS() || setupConfigFromApServer();
}
