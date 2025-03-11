#include <ArduinoJson.h>
#include "Constants.h"

long lastFetchedPrice = -FETCH_FIAT_BALANCE_PERIOD_MS;

float lastBtcPrice = NOT_SPECIFIED;

// https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd
// returns: {"bitcoin":{"myr":429094}}
// returns: {"bitcoin":{"usd":96474}}
float getBitcoinPriceCoingecko() {
  #ifdef DEBUG
  return 60456;
  #endif

  String btcPriceCurrency = String(btcPriceCurrencyChar);
  btcPriceCurrency.toLowerCase();

  if (lastBtcPrice != NOT_SPECIFIED && millis() - lastFetchedPrice < FETCH_FIAT_BALANCE_PERIOD_MS) {
    Serial.println("Fiat price was fetched recently, returning cached value.");
    return lastBtcPrice;
  } else {
    lastFetchedPrice = millis();
    Serial.println("Getting Bitcoin price from coingecko...");
  }

  #ifdef DEBUG
  Serial.println("Mocking getBitcoinPrice:"); return 30000.2;
  #endif

  // Get the data
  String path = "/api/v3/simple/price?ids=bitcoin&vs_currencies=" + btcPriceCurrency;
  String priceData = getEndpointData("api.coingecko.com", path, false);
  DynamicJsonDocument doc(8192); // the size of the list of links is unknown so don't skimp here

  DeserializationError error = deserializeJson(doc, priceData);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return NOT_SPECIFIED;
  }

  Serial.println("Extracting bitcoin price from received data");
  lastBtcPrice = doc["bitcoin"][btcPriceCurrency];

  if (lastBtcPrice == 0.0) {
    Serial.println("BTC Price not found, returning NOT_SPECIFIED");
    return (float)NOT_SPECIFIED;
  }

  Serial.println("BTC Price: " + String(lastBtcPrice, 0));
  return lastBtcPrice;
}

/* Unused because API was broken:
float getBitcoinPriceCoindesk() {
  String btcPriceCurrency = String(btcPriceCurrencyChar);
  btcPriceCurrency.toUpperCase();
  Serial.println("Getting Bitcoin price...");

  #ifdef DEBUG
  Serial.println("Mocking getBitcoinPrice:"); return 30000.2;
  #endif

  // Get the data
  String path = "/v1/bpi/currentprice/" + btcPriceCurrency + ".json";
  String priceData = getEndpointData("api.coindesk.com", path, false);
  DynamicJsonDocument doc(8192); // the size of the list of links is unknown so don't skimp here

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
*/
