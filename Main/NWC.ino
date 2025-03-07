#include <NostrCommon.h>
#include "esp32/ESP32Platform.h"

#include "NostrTransport.h"
#include "services/NWC.h"

#define NWC_GET_NEXT_TRANSACTION_TIMEOUT_MS 15 * 1000
#define NWC_GET_BALANCE_TIMEOUT_MS 20 * 1000

nostr::NWC *nwc = NULL;
nostr::Transport *transport;

unsigned long long maxtime; // if maxtime is 0, then it will default to "now", so it will fetch the latest transaction, and work backwards from there
unsigned long long requestedTime;

int paymentsFetched;
int paymentsToFetch;
bool reachedEndOfTransactionsList;
bool fetchDoneNotified;

bool gotBalance;

long timeOfGetBalance;
long timeOfGetNextTransaction;

bool canUseNWC() {
  return isConfigured(nwcURL);
}

String extractPlainTextFromTransactionDescription(const String &input) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, input);

    if (!error && doc.is<JsonArray>()) {
        JsonArray outerArray = doc.as<JsonArray>();
        for (JsonVariant rowVariant : outerArray) {
            if (rowVariant.is<JsonArray>()) {
                JsonArray row = rowVariant.as<JsonArray>();
                for (size_t i = 0; i < row.size(); i++) {
                    if (row[i].as<String>() == "text/plain" && i + 1 < row.size()) {
                        return row[i + 1].as<String>();
                    }
                }
            }
        }
    }
    return input; // Return the original input if it's not valid JSON or no "text/plain" found
}


void getNextTransaction() {
  Serial.println("Doing listTransactions for maxtime " + String(maxtime) + ":");

  try {
    nwc->listTransactions(0,maxtime,1,0,false,"incoming", [&](nostr::ListTransactionsResponse resp) {
        size_t numTransactions = resp.transactions.size();
        Serial.println("[!] listTransactions result for maxtime " + String(maxtime) + " has " + String(numTransactions) + " transactions: ");
        if (numTransactions == 0) reachedEndOfTransactionsList = true;

        for (auto transaction : resp.transactions) {
          Serial.println("=> Got transaction: " + String(transaction.amount) + " msat createdAt: " + String(transaction.createdAt) + " with description: '" + transaction.description + "'");         
          maxtime = transaction.createdAt-1;
          // Payment always has an amount
          long long amount = transaction.amount; // long long to support amounts above 999999000 millisats
          long amountSmaller = amount / 1000; // millisats to sats

          String paymentAmount(amountSmaller);
          String units = "sats";
          if (amountSmaller < 2) units = "sat";

          String paymentDetail = paymentAmount + " " + units;

          String paymentComment(extractPlainTextFromTransactionDescription(transaction.description));
          if (paymentComment.length() == 0) {
            paymentDetail += "!";
          } else {
            paymentDetail += ": " + paymentComment;
          }
          appendPayment(paymentDetail);
        }
    }, [](String err, String errMsg) { Serial.println("[!] listTransactions Error: " + err + " " + errMsg); });

  } catch (std::exception &e) {
      Serial.println("[!] Exception: " + String(e.what()));
  }
}

void loop_nwc() {
  if (nwc == NULL) return; // not initialized

  nwc->loop();

  // check if it times out
  if (!getBalanceDone() && millis() - timeOfGetBalance > NWC_GET_BALANCE_TIMEOUT_MS) {
    Serial.println("get_balance timed out!");
    setGetBalanceDone(true);
  }

  // if we can request the next one AND we need to fetch the next one AND we haven't reached the end:
  if (paymentsFetched < paymentsToFetch && !reachedEndOfTransactionsList) {
    if (maxtime != requestedTime) {
      requestedTime = maxtime;
      paymentsFetched++;
      timeOfGetNextTransaction = millis();
      getNextTransaction();
    } else if (millis() - timeOfGetNextTransaction > NWC_GET_NEXT_TRANSACTION_TIMEOUT_MS) {
      // for some reason no reply came in, so just cancel the whole thing
      // even better would be to redo the request a few times first, but that's more complex
      reachedEndOfTransactionsList = true;
    } // still waiting for a reply, do nothing...
  } else { // fetched enough payments or reached end of list
    if (!fetchDoneNotified) {
      fetchDoneNotified = true;
      setFetchedPaymentsDone(true);
    } // else already notified that the fetch is done
  }
}

void setup_nwc() {
  Serial.println("Initializing time using NTP...");
  nostr::esp32::ESP32Platform::initTime("pool.ntp.org");

  Serial.print("UTC time: ");
  printLocalTime();

  Serial.println("Setting local timezone...");
  setTimeZone(timezone);

  Serial.print("Local time: ");
  printLocalTime();

  Serial.println("Initializing Nostr...");
  nostr::esp32::ESP32Platform::initNostr(true);

  transport = nostr::esp32::ESP32Platform::getTransport();
  nwc = new nostr::NWC(transport, nwcURL);
}

void nwc_getBalance() {
  timeOfGetBalance = millis();
  setGetBalanceDone(false);
  nwc->getBalance([&](nostr::GetBalanceResponse resp) {
    Serial.println("[!] Balance: " + String(resp.balance) + " msatoshis");
    setBalance(resp.balance/1000);
    setGetBalanceDone(true);
  }, [](String err, String errMsg) {
    Serial.println("[!] Error: " + err + " " + errMsg);
  });
}


void fetchNWCPayments(int max_payments) {
  paymentsFetched = 0;
  paymentsToFetch = max_payments;
  reachedEndOfTransactionsList = false;
  fetchDoneNotified = false;
  maxtime = 0; // start from the "now" and go backwards
  requestedTime = NOT_SPECIFIED;
}
