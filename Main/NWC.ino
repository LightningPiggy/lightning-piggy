#include <NostrCommon.h>
#include "esp32/ESP32Platform.h"

#include "NostrTransport.h"
#include "services/NWC.h"

#define NWC_GET_NEXT_TRANSACTION_TIMEOUT_MS 30 * 1000

nostr::NWC *nwc;
nostr::Transport *transport;

unsigned long long maxtime; // if maxtime is 0, then it will default to "now", so it will fetch the latest transaction, and work backwards from there
unsigned long long requestedTime;

int paymentsFetched;
int paymentsToFetch;
bool reachedEndOfTransactionsList;
bool fetchDoneNotified;

String nwcPayments[MAX_PAYMENTS];
int nrofNWCPayments;

long timeOfGetNextTransaction;

bool canUseNWC() {
  return isConfigured(nwcURL);
}

int getNrofNWCPayments() {
  return nrofNWCPayments;
}

String getNWCPayment(int index) {
  return nwcPayments[index];
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

            String paymentComment(transaction.description);
            if (paymentComment.length() == 0) {
              paymentDetail += "!";
            } else {
              paymentDetail += ": " + paymentComment;
            }
            addNWCpayment(paymentDetail);
        }
    }, [](String err, String errMsg) { Serial.println("[!] listTransactions Error: " + err + " " + errMsg); });

  } catch (std::exception &e) {
      Serial.println("[!] Exception: " + String(e.what()));
  }
}

void loop_nwc() {
  if (!nwc) return; // not initialized

  nwc->loop();
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
      receivedPayments();
    } // else already notified that the fetch is done
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
  nwc->getBalance([&](nostr::GetBalanceResponse resp) {
    Serial.println("[!] Balance: " + String(resp.balance) + " msatoshis");
    receivedWalletBalance(resp.balance/1000);
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
  nrofNWCPayments = 0;
}

void addNWCpayment(String toadd) {
  nwcPayments[nrofNWCPayments] = toadd;
  if (nrofNWCPayments<MAX_PAYMENTS) nrofNWCPayments++;
  Serial.println("After adding NWC payment, the list contains:" + stringArrayToString(nwcPayments, nrofNWCPayments));

}
