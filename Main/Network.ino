#include "esp_wifi.h"
#include "esp_mac.h"

#include <WiFiClientSecure.h>

#define HTTP_BUFFER_SIZE 1024

long lastWebsocketConnectionAttempt = -TIME_BETWEEN_WEBSOCKET_CONNECTION_ATTEMPTS;

esp_netif_t* softap_netif;
WebSocketsClient webSocket;

long wifiStartTime = 0;
String lastWifiError = "";

int system_event_names_count = 26;
const char * system_event_names[] = { "WIFI_READY", "SCAN_DONE", "STA_START", "STA_STOP", "STA_CONNECTED", "STA_DISCONNECTED", "STA_AUTHMODE_CHANGE", "STA_GOT_IP", "STA_LOST_IP", "STA_WPS_ER_SUCCESS", "STA_WPS_ER_FAILED", "STA_WPS_ER_TIMEOUT", "STA_WPS_ER_PIN", "STA_WPS_ER_PBC_OVERLAP", "AP_START", "AP_STOP", "AP_STACONNECTED", "AP_STADISCONNECTED", "AP_STAIPASSIGNED", "AP_PROBEREQRECVED", "GOT_IP6", "ETH_START", "ETH_STOP", "ETH_CONNECTED", "ETH_DISCONNECTED", "ETH_GOT_IP", "MAX"};
const char * system_event_reasons2[] = { "UNSPECIFIED", "AUTH_EXPIRE", "AUTH_LEAVE", "ASSOC_EXPIRE", "ASSOC_TOOMANY", "NOT_AUTHED", "NOT_ASSOCED", "ASSOC_LEAVE", "ASSOC_NOT_AUTHED", "DISASSOC_PWRCAP_BAD", "DISASSOC_SUPCHAN_BAD", "UNSPECIFIED", "IE_INVALID", "MIC_FAILURE", "4WAY_HANDSHAKE_TIMEOUT", "GROUP_KEY_UPDATE_TIMEOUT", "IE_IN_4WAY_DIFFERS", "GROUP_CIPHER_INVALID", "PAIRWISE_CIPHER_INVALID", "AKMP_INVALID", "UNSUPP_RSN_IE_VERSION", "INVALID_RSN_IE_CAP", "802_1X_AUTH_FAILED", "CIPHER_SUITE_REJECTED", "BEACON_TIMEOUT", "NO_AP_FOUND", "AUTH_FAIL", "ASSOC_FAIL", "HANDSHAKE_TIMEOUT", "CONNECTION_FAIL" };
#define reason2str(r) ((r>176)?system_event_reasons2[r-176]:system_event_reasons2[r-1])


void wifiEventCallback(WiFiEvent_t eventid, WiFiEventInfo_t info) {
  // Regular flow of events is: 0, 2, 7 (and then 3 when the device goes to sleep)
  String wifiStatus = "WiFi Event ID " + String(eventid);
  if (eventid > 0 && eventid < system_event_names_count) wifiStatus += " which means: " + String(system_event_names[eventid]);
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
     lastWifiError = details;
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
    setNextRefreshBalanceAndPayments(true);
  }

  Serial.println(details);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
    Serial.printf("station " MACSTR" join, AID=%d\n", MAC2STR(event->mac), event->aid);
  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
    Serial.printf("station " MACSTR" leave, AID=%d, reason=%d\n", MAC2STR(event->mac), event->aid, event->reason);
  }
}

bool connectWifiAsync() {
  if (!isConfigured(ssid)) return false;

  wifi_ap_stop(); // make sure there is no Access Point active, otherwise it might go into AP+STA mode
  Serial.println("Connecting to " + String(ssid));
  lastWifiError = "";
  wifiStartTime = millis();
  WiFi.onEvent(wifiEventCallback);
  WiFi.persistent(false); // trigger esp_wifi_set_storage(WIFI_STORAGE_RAM) to workaround no reply issue in a159x36/qemu
  WiFi.begin(ssid, password);
  return true;
}

// returns true if need to keep waiting, false if done waiting
bool keepWaitingWifi() {
  if (wifiConnected()) {
    Serial.print("WiFi connected. IP address: "); Serial.println(WiFi.localIP());
    return false;
  } // else wifi not connected:

  if (lastWifiError != "") {
    // Write the disconnection reason to the display for troubleshooting
    displayFit(lastWifiError, 0, 0, displayWidth(), 40, 1);
    lastWifiError = "";
  }

  if (millis() - wifiStartTime > WIFI_CONNECT_TIMEOUT_SECONDS*1000) {
    Serial.println("WARNING: Wifi connection did not succeed before timeout!");
    return false;
  }

  return true;  // keep waiting
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
  if (!client.connected()) {
    client.stop();
    Serial.println("Connection failed, returning empty reply...");
    return "";
  }

  String request = "GET " + endpointUrl + " HTTP/1.1\r\n" +
               "Host: " + String(host) + "\r\n" +
               "User-Agent: " + getFullVersion() + "\r\n" +
               "Content-Type: application/json\r\n" +
               "Connection: close\r\n";
  if (sendApiKey) request += "X-Api-Key: " + String(lnbitsInvoiceKey) + "\r\n";
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
        uint8_t buff[HTTP_BUFFER_SIZE] = {0}; // zero initialize buffer to have 0x00 at the end
        int readNow = min(bytesToRead - bytesRead,HTTP_BUFFER_SIZE-1); // leave one byte for the 0x00 at the end
        //Serial.println("Reading bytes: " + String(readNow));
        int thisBytesRead = client.read(buff, readNow);
        //Serial.println("thisBytesRead = " + String(thisBytesRead));
        if (thisBytesRead > 0) {
          bytesRead += thisBytesRead;
          String stringBuff = (char*)buff;
          line += stringBuff;
        } else {
          //Serial.println("No bytes available from HTTPS, waiting a bit...");
          //delay(42);
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
  client.stop();

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
  String url = websocketApiUrl + String(lnbitsInvoiceKey);
  int lnbitsPortInteger = getConfigValueAsInt((char*)lnbitsPort, DEFAULT_LNBITS_PORT);
  Serial.println("Trying to connect websocket: wss://" + String(lnbitsHost) + ":" + String(lnbitsPortInteger) + url);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(1000);
  webSocket.beginSSL(lnbitsHost, lnbitsPortInteger, url);
}

void parseWebsocketText(String text) {
  Serial.println("Parsing websocket text: " + text);
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
    prependPayment(paymentDetail);
    setBalance(walletBalance+balanceBiasInt);
    piggyMode = PIGGYMODE_STARTED_STA_RECEIVED_BALANCE;
  } else {
    Serial.println("Websocket update did not contain payment, ignoring...");
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t wslength) {
    Serial.print("WebSocket Event Type: ");
    Serial.print(type); // Print the type as an integer
    Serial.println(" and length: " + String(wslength));

    if (payload != nullptr && wslength > 0) { // Check if payload is not null and has data
        Serial.print("Websocket Payload: ");
        if (type != WStype_CONNECTED && type != WStype_TEXT) {
          // Print payload as hex
          for (size_t i = 0; i < wslength; i++) {
              Serial.print("0x");
              if (payload[i] < 0x10) Serial.print("0"); // Leading zero for single-digit hex
              Serial.print(payload[i], HEX);
              Serial.print(" ");
          }
          Serial.println(); // Newline after printing payload
        } // else use specific payload print below
    }
    String payloadStr = "";
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[WSc] Disconnected!\r\n");
            Serial.println("payload : " + String((int)payload));
            break;
        case WStype_CONNECTED:
            Serial.printf("[WSc] Connected to url: %s, waiting for incoming payments...\r\n",  payload);
            // No need: webSocket.sendTXT("Connected"); // send message to server when Connected
            break;
        case WStype_TEXT:
            payloadStr = String((char*)payload);
            Serial.println("Received data from socket: " + payloadStr);
            parseWebsocketText(payloadStr);
            break;
        case WStype_ERROR:
            Serial.printf("[WSc] error!\r\n");
            break;
        case WStype_PING:
            Serial.println("Websocket ping.");
            break;
        case WStype_PONG:
            Serial.println("Websocket pong.");
            break;
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
        case WStype_BIN:
            Serial.printf("[WSc] other fragment!\r\n");
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
      //delay(100);
    } else {
      Serial.println("Websocket is not connected, not disconnecting.");
    }
}

void wifi_init_softap(void)
{
    disconnectWifi(); // make sure STA mode is stopped, otherwise it might go into AP+STA mode
    int delaytime = 10; // 5 might be too short (0816 and crash) so go with 10. 10 crashed so go for 15?
    int counter = 0;
    Serial.println(counter++); delay(delaytime);
    ESP_ERROR_CHECK(esp_netif_init()); delay(delaytime); // if he hangs a long time after this, that seems to be a good sign...
    Serial.println(counter++); delay(delaytime);
    //ESP_ERROR_CHECK(esp_event_loop_create_default()); // this crashes if STA was used before
    //Serial.println(counter++);delay(delaytime);
    Serial.println("skipped esp_event_loop_create_default for now!");
    // esp_event_loop_create_default(); with an error check instead of abort() would also work but it doesn't seem necessary, probably in arduino context

    esp_event_loop_create_default();
    Serial.println(counter++);delay(delaytime);
    softap_netif = esp_netif_create_default_wifi_ap(); // causes conflict with other WiFi.softAP()
    Serial.println(counter++);delay(delaytime);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    Serial.println(counter++);delay(delaytime);
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    Serial.println(counter++);delay(delaytime);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    Serial.println(counter++);delay(delaytime);

    Serial.println(counter++);delay(delaytime);
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config)); // Ensuring all fields are initialized
    Serial.println(counter++);delay(delaytime);

    strncpy(reinterpret_cast<char*>(wifi_config.ap.ssid), ACCESS_POINT_SSID, sizeof(wifi_config.ap.ssid) - 1);
    Serial.println(counter++);delay(delaytime);
    wifi_config.ap.ssid_len = strlen(ACCESS_POINT_SSID);
    Serial.println(counter++);delay(delaytime);
    wifi_config.ap.channel = ACCESS_POINT_CHANNEL;
    Serial.println(counter++);delay(delaytime);
    strncpy(reinterpret_cast<char*>(wifi_config.ap.password), ACCESS_POINT_PASS, sizeof(wifi_config.ap.password) - 1);
    Serial.println(counter++);delay(delaytime);
    wifi_config.ap.max_connection = ACCESS_POINT_MAX_STA_CONN;
    Serial.println(counter++);delay(delaytime);
    wifi_config.ap.pmf_cfg.required = true;
    Serial.println(counter++);delay(delaytime);

    if (strlen(ACCESS_POINT_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    Serial.println(counter++);delay(delaytime);
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    Serial.println(counter++);delay(delaytime);
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    Serial.println(counter++);delay(delaytime);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    Serial.println(counter++);delay(delaytime);
    ESP_ERROR_CHECK(esp_wifi_start());
    Serial.println(counter++);delay(delaytime);

    Serial.println("wifi_init_softap finished. SSID: '" + String(ACCESS_POINT_SSID) + "', password '" + String(ACCESS_POINT_PASS) + "', channel: " + String(ACCESS_POINT_CHANNEL));
    Serial.println(counter++);delay(delaytime);
}

// This should be harmless if the AP is not started
void wifi_ap_stop() {
  Serial.println("Stopping Access Point...");
  esp_netif_destroy_default_wifi(softap_netif);
}
