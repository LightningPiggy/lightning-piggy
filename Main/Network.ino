#include <WiFiClientSecure.h>

#define BUFFSIZE 1024

WebSocketsClient webSocket;
long lastWebsocketConnectionAttempt = -TIME_BETWEEN_WEBSOCKET_CONNECTION_ATTEMPTS;

const char * system_event_names[] = { "WIFI_READY", "SCAN_DONE", "STA_START", "STA_STOP", "STA_CONNECTED", "STA_DISCONNECTED", "STA_AUTHMODE_CHANGE", "STA_GOT_IP", "STA_LOST_IP", "STA_WPS_ER_SUCCESS", "STA_WPS_ER_FAILED", "STA_WPS_ER_TIMEOUT", "STA_WPS_ER_PIN", "STA_WPS_ER_PBC_OVERLAP", "AP_START", "AP_STOP", "AP_STACONNECTED", "AP_STADISCONNECTED", "AP_STAIPASSIGNED", "AP_PROBEREQRECVED", "GOT_IP6", "ETH_START", "ETH_STOP", "ETH_CONNECTED", "ETH_DISCONNECTED", "ETH_GOT_IP", "MAX"};
const char * system_event_reasons2[] = { "UNSPECIFIED", "AUTH_EXPIRE", "AUTH_LEAVE", "ASSOC_EXPIRE", "ASSOC_TOOMANY", "NOT_AUTHED", "NOT_ASSOCED", "ASSOC_LEAVE", "ASSOC_NOT_AUTHED", "DISASSOC_PWRCAP_BAD", "DISASSOC_SUPCHAN_BAD", "UNSPECIFIED", "IE_INVALID", "MIC_FAILURE", "4WAY_HANDSHAKE_TIMEOUT", "GROUP_KEY_UPDATE_TIMEOUT", "IE_IN_4WAY_DIFFERS", "GROUP_CIPHER_INVALID", "PAIRWISE_CIPHER_INVALID", "AKMP_INVALID", "UNSUPP_RSN_IE_VERSION", "INVALID_RSN_IE_CAP", "802_1X_AUTH_FAILED", "CIPHER_SUITE_REJECTED", "BEACON_TIMEOUT", "NO_AP_FOUND", "AUTH_FAIL", "ASSOC_FAIL", "HANDSHAKE_TIMEOUT", "CONNECTION_FAIL" };
#define reason2str(r) ((r>176)?system_event_reasons2[r-176]:system_event_reasons2[r-1])

void wifiEventCallback(WiFiEvent_t eventid, WiFiEventInfo_t info) {
  // Regular flow of events is: 0, 2, 7 (and then 3 when the device goes to sleep)
  String wifiStatus = "WiFi Event ID " + String(eventid) + " which means: " + String(system_event_names[eventid]);
  Serial.print(wifiStatus);

  String details = "";

  // Full list at ~/.arduino15/packages/esp32/hardware/esp32/1.0.6/tools/sdk/include/esp32/esp_event_legacy.h
  if(eventid == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    uint8_t reason = info.wifi_sta_disconnected.reason;
    details = "Wifi error " + String(reason) + ": " + String(reason2str(reason));

    if (reason == WIFI_REASON_MIC_FAILURE || reason == WIFI_REASON_AUTH_FAIL || reason == WIFI_REASON_AUTH_EXPIRE || reason == WIFI_REASON_ASSOC_EXPIRE || reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT) {
      details += ". Wrong password? Max. WPA2 security.";
    } else if (reason == WIFI_REASON_NO_AP_FOUND) {
      details += ". Wrong SSID? Only 2.4Ghz supported.";
    }

    if (reason != WIFI_REASON_ASSOC_LEAVE) { // ignore intentional disconnections (such as before deepsleep)
	    // Write the disconnection reason to the display for troubleshooting
	    displayFit(details, 0, 0, displayWidth(), 40, 1);
    }

    // Reboot or longsleep after unrecoverable errors OR if it's been trying for a long time already.
    // Full list at ~/.arduino15/packages/esp32/hardware/esp32/1.0.6/tools/sdk/include/esp32/esp_wifi_types.h
    // Especially:
    // - NO_AP_FOUND
    // - AUTH_FAIL
    // - ASSOC_FAIL
    // - AUTH_EXPIRE : wrong password or slow connection but not an end state
    //
    // - 4WAY_HANDSHAKE_TIMEOUT? some kind of end state, happens with wrong password
    // - ASSOC_EXPIRE? some kind of end state, after 7 AUTH_EXPIREs
    // - AUTH_LEAVE? end state, happened when adding and changing password during connection
    // - WIFI_REASON_MIC_FAILURE? with wrong password, endstate

    // Boot takes around 11 seconds until wifi connection.
    if ((reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT || reason == WIFI_REASON_ASSOC_EXPIRE || reason == WIFI_REASON_AUTH_LEAVE || reason == WIFI_REASON_MIC_FAILURE)
      || (millis() > 25*1000)) {
      Serial.println("WARNING: This wifi error is unrecoverable or it's taking too long, needs restart.");
      // If the next watchdog restart will trigger the max and long sleep,
      // then do that right now, so the error on-screen stays for troubleshooting.
      if (nextWatchdogRebootWillReachMax()) {
        longsleepAfterMaxWatchdogReboots();
      } else {
        // Allow the watchdog to restart soon.
        short_watchdog_timeout();
      }
    } // else it's a non-final error and still early after boot so do nothing
  } else if(eventid == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
      details = "Obtained IP address: " + ipToString(WiFi.localIP());
  } else if (eventid == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
    Serial.println("Got ARDUINO_EVENT_WIFI_STA_CONNECTED; flagging balance and payments for refresh to clear any error messages that might linger on the display.");
    nextRefreshBalanceAndPayments();
  }

  Serial.println(details);

}

void connectWifi() {
  Serial.println("Connecting to " + String(ssid));
  WiFi.onEvent(wifiEventCallback);
  WiFi.begin(ssid, password);
  long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) { // there's no max attempts but the watchdog should trigger a restart
      delay(1000);
      Serial.print(".");
      if ((millis() - startTime) > WIFI_CONNECT_TIMEOUT_SECONDS*1000) {
        displayWifiIssue(WIFI_CONNECT_TIMEOUT_SECONDS);
        hibernateDependingOnBattery();
      }
  }
  Serial.print("WiFi connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void disconnectWifi() {
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_OFF);
  delay(500);
}

bool wifiConnected() {
  return (WiFi.status() == WL_CONNECTED);
}

// Take measurements of the Wi-Fi strength and return the average result.
// 100 measurements takes 2 seconds so 20ms per measurement
int getStrength(int points){
    #ifdef DEBUG
      delay(points*20);
      return 42;
    #endif

    long rssi = 0;
    long averageRSSI = 0;

    for (int i=0;i < points;i++){
        rssi += WiFi.RSSI();
        delay(20);
    }

    averageRSSI = rssi/points;
    return averageRSSI;
}

/*
  RSSI Value Range WiFi Signal Strength:
  ======================================
  RSSI > -30 dBm  Amazing
  RSSI < – 55 dBm   Very good signal
  RSSI < – 67 dBm  Fairly Good
  RSSI < – 70 dBm  Okay
  RSSI < – 80 dBm  Not good
  RSSI < – 90 dBm  Extremely weak signal (unusable)

 */
int strengthPercent(float strength) {
  int strengthPercent = 100 + strength;
  // ESP32 returns RSSI above 0 sometimes, so limit to 99% max:
  if (strengthPercent >= 100) strengthPercent = 99;
  return strengthPercent;
}

/**
 * @brief GET data from a HTTPS URL
 *
 * @param endpointUrl 
 * @return String 
 */
String getEndpointData(const char * host, String endpointUrl, bool sendApiKey) {
  int connectionAttempts = 0;

  int lnbitsPortInteger = getConfigValueAsInt((char*)lnbitsPort, DEFAULT_LNBITS_PORT);
  if (strncmp(host,lnbitsHost,MAX_CONFIG_LENGTH)!=0) lnbitsPortInteger = 443; // only use lnbitsPort for lnbitsHost

  Serial.println("Fetching " + endpointUrl + " from " + String(host) + " on port " + String(lnbitsPortInteger));

  WiFiClientSecure client;
  client.setInsecure(); // see https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFiClientSecure/README.md
  client.setHandshakeTimeout(HTTPS_TIMEOUT_SECONDS);
  client.setTimeout(HTTPS_TIMEOUT_SECONDS * 1000);

  while (feed_watchdog() && connectionAttempts < MAX_HTTPS_CONNECTION_ATTEMPTS && !client.connect(host, lnbitsPortInteger)) {
      connectionAttempts++;
      Serial.println("Couldn't connect to " + String(host) + " on port " + String(lnbitsPortInteger) + " (attempt " + String(connectionAttempts) + "/" + String(MAX_HTTPS_CONNECTION_ATTEMPTS) + ")");
  }
  if (!client.connected()) return "";

  String request = "GET " + endpointUrl + " HTTP/1.1\r\n" +
               "Host: " + String(host) + "\r\n" +
               "User-Agent: " + getFullVersion() + "\r\n" +
               "Content-Type: application/json\r\n" +
               "Connection: close\r\n";
  if (sendApiKey) request += "X-Api-Key: " + String(invoiceKey) + "\r\n";
  request += "\r\n";

  feed_watchdog();
  client.print(request);

  long maxTime = millis() + HTTPS_TIMEOUT_SECONDS * 1000;

  int chunked = 0;
  String line = "";
  while (client.connected() && (millis() < maxTime))
  {
    line = client.readStringUntil('\n');
    line.toLowerCase();
    if (line == "\r")
    {
      break;
    } else if (line == "transfer-encoding: chunked\r") {
      Serial.println("HTTP chunking enabled");
      chunked = 1;
    }
  }

  if (chunked == 0) {
    line = client.readString();
  } else {
    // chunked means first length, then content, then length, then content, until length == "0"
    String lengthline = client.readStringUntil('\n');
    Serial.println("chunked reader got length line: '" + lengthline + "'");

    line = "";
    while (lengthline != "0\r" && (millis() < maxTime)) {
      const char *lengthLineChar = lengthline.c_str();
      int bytesToRead = strtol(lengthLineChar, NULL, 16);
      Serial.println("bytesToRead = " + String(bytesToRead));

      int bytesRead = 0;
      while (bytesRead < bytesToRead && (millis() < maxTime)) { // stop if less than max bytes are read
        uint8_t buff[BUFFSIZE] = {0}; // zero initialize buffer to have 0x00 at the end
        int readNow = min(bytesToRead - bytesRead,BUFFSIZE-1); // leave one byte for the 0x00 at the end
        //Serial.println("Reading bytes: " + String(readNow));
        int thisBytesRead = client.read(buff, readNow);
        //Serial.println("thisBytesRead = " + String(thisBytesRead));
        if (thisBytesRead > 0) {
          bytesRead += thisBytesRead;
          String stringBuff = (char*)buff;
          line += stringBuff;
        } else {
          Serial.println("No bytes available from HTTPS, waiting a bit...");
          delay(42);
        }
        //Serial.println("chunked total reply = '" + reply + "'");
      }

      // skip \r\n
      client.read();
      client.read();

      // next chunk length
      lengthline = client.readStringUntil('\n');
      Serial.println("chunked reader got length line: '" + lengthline + "'");
    }

  }
  //Serial.println("returning total chunked reply = '" + reply + "'");
  feed_watchdog(); // after successfully completing this long-running and potentially hanging operation, it's a good time to feed the watchdog
  return line;
}

void connectWebsocket() {
  disconnectWebsocket(); // make sure it's disconnected
  if (!wifiConnected()) {
    Serial.println("Not connecting websocket because wifi is not connected.");
    return;
  }
  if ((millis() - lastWebsocketConnectionAttempt) < TIME_BETWEEN_WEBSOCKET_CONNECTION_ATTEMPTS) {
    Serial.println("Not connecting websocket because it was attempted just recently.");
    return;
  }
  lastWebsocketConnectionAttempt = millis();
  // wss://demo.lnpiggy.com/api/v1/ws/<invoice read key>
  String url = websocketApiUrl + String(invoiceKey);
  int lnbitsPortInteger = getConfigValueAsInt((char*)lnbitsPort, DEFAULT_LNBITS_PORT);
  Serial.println("Trying to connect websocket: wss://" + String(lnbitsHost) + ":" + String(lnbitsPortInteger) + url);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(1000);
  webSocket.beginSSL(lnbitsHost, lnbitsPortInteger, url);
}

void parseWebsocketText(String text) {
  String returnValue = "";
  DynamicJsonDocument doc(4096); // 4096 bytes is plenty for just the wallet details (id, name and balance info)

  DeserializationError error = deserializeJson(doc, text);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  int walletBalance = doc["wallet_balance"];
  int balanceBiasInt = getConfigValueAsInt((char*)balanceBias, 0);
  Serial.println("Wallet now contains " + String(walletBalance) + " sats and balance bias of " + String(balanceBiasInt) + " sats.");

  if (doc["payment"]) {
    resetLastPaymentReceivedMillis();
    String paymentDetail = paymentJsonToString(doc["payment"].as<JsonObject>());
    Serial.println("Websocket update with paymentDetail: " + paymentDetail);
    addLNURLpayment(paymentDetail);
    updateBalanceAndPayments(xBeforeLNURLp, walletBalance+balanceBiasInt, false);
  } else {
    Serial.println("Websocket update did not contain payment, ignoring...");
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t wslength) {
    String payloadStr = "";
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[WSc] Disconnected!\n");
            Serial.println(" and length: " + String(wslength));
            Serial.println("payload : " + String((int)payload));
            break;
        case WStype_CONNECTED:
            Serial.printf("[WSc] Connected to url: %s, waiting for incoming payments...\n",  payload);
            // No need: webSocket.sendTXT("Connected"); // send message to server when Connected
            break;
        case WStype_TEXT:
            payloadStr = String((char*)payload);
            Serial.println("Received data from socket: " + payloadStr);
            parseWebsocketText(payloadStr);
            break;
        case WStype_ERROR:
            Serial.printf("[WSc] error!\n");
            break;
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            Serial.printf("[WSc] other fragment!\n");
            break;
    }
}

void loop_websocket() {
    webSocket.loop();
}

bool isWebsocketConnected() {
  return webSocket.isConnected();
}

void disconnectWebsocket() {
    if (webSocket.isConnected()) {
      Serial.println("Websocket is connected, disconnecting...");
      webSocket.disconnect();
      delay(100);
    } else {
      Serial.println("Websocket is not connected, not disconnecting.");
    }
}
