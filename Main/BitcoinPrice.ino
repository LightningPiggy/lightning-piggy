#include <ArduinoJson.h>
#include "Constants.h"

float getBitcoinPrice() {
  String btcPriceCurrency = String(btcPriceCurrencyChar);
  Serial.println("Getting Bitcoin price...");

  #ifdef DEBUG
  Serial.println("Mocking getBitcoinPrice:"); return 30000.2;
  #endif

  // Get the data
  String path = "/v1/bpi/currentprice/" + btcPriceCurrency + ".json";
  String priceData = getEndpointData("api.coindesk.com", path, false);
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, priceData);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return NOT_SPECIFIED;
  }

  Serial.println("Extracting bitcoin price from received data");

  float btcPrice = doc["bpi"][btcPriceCurrency]["rate_float"];

  if (btcPrice == 0.0) {
    Serial.println("BTC Price not found, returning NOT_SPECIFIED");
    return (float)NOT_SPECIFIED;
  }

  Serial.println("BTC Price: " + String(btcPrice, 0));
  return btcPrice;
}
