#include <NostrCommon.h>
#include "esp32/ESP32Platform.h"

#include "NostrTransport.h"
#include "services/NWC.h"

nostr::NWC *nwc;
nostr::Transport *transport;

unsigned long long maxtime = 0; // if maxtime is 0, then it will default to "now", so it will fetch the latest transaction, and work backwards from there
unsigned long long requestedTime = -1;

void getNextTransaction() {
  Serial.println("Doing listTransactions for maxtime " + String(maxtime) + ":");

  try {

    nwc->listTransactions(0,maxtime,1,0,false,"incoming", [&](nostr::ListTransactionsResponse resp) {
        Serial.println("[!] listTransactions result for maxtime " + String(maxtime) + ":");
        for (auto transaction : resp.transactions) {
          Serial.println("=> Got transaction: " + String(transaction.amount) + " msat createdAt: " + String(transaction.createdAt) + " with description: '" + transaction.description + "'");         
          maxtime = transaction.createdAt-1;
        }
    }, [](String err, String errMsg) { Serial.println("[!] listTransactions Error: " + err + " " + errMsg); });

  } catch (std::exception &e) {
      Serial.println("[!] Exception: " + String(e.what()));
  }
}

void loop_nwc() {
  nwc->loop();
  if (maxtime != requestedTime) {
    requestedTime = maxtime;
    getNextTransaction();
  }
}

void setup_nwc() {
  Serial.println("Init time");
  nostr::esp32::ESP32Platform::initTime("pool.ntp.org");

  Serial.println("Init Nostr");
  nostr::esp32::ESP32Platform::initNostr(true);

  transport = nostr::esp32::ESP32Platform::getTransport();
  nwc = new nostr::NWC(transport, nwcURL);
}

void nwc_getBalance() {
  nwc->getBalance([&](nostr::GetBalanceResponse resp) { Serial.println("[!] Balance: " + String(resp.balance) + " msatoshis"); },
      [](String err, String errMsg) { Serial.println("[!] Error: " + err + " " + errMsg); });
}
