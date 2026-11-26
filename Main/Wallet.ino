// Wallet abstraction

int oldBalance = NOT_SPECIFIED;
int currentBalance = NOT_SPECIFIED;

String payments[MAX_PAYMENTS];
int nrOfPayments = NOT_SPECIFIED;

// The payments shown before the current fetch, to keep them on the display if it fails.
String previousPayments[MAX_PAYMENTS];
int nrOfPreviousPayments = 0;
bool paymentsFetchFailed = false;

bool fetchedAllPayments;
bool getBalanceIsDone;

bool hasWalletConfig() {
  return (walletToUse() != WALLET_NONE);
}

int walletToUse() {
  if (canUseLNBits()) {
    return WALLET_LNBITS;
  } else if (canUseNWC()) {
    return WALLET_NWC;
  } else {
    return WALLET_NONE;
  }
}

void getWalletBalanceAsync() {
  oldBalance = currentBalance;
  if (walletToUse() == WALLET_LNBITS) {
    int balance = getWalletBalance();
    if (balance != NOT_SPECIFIED) {
      currentBalance = balance;
    } else {
      Serial.println("WARNING: could not get the balance, keeping the previous one on the display.");
    }
    setGetBalanceDone(true);
  } else if (walletToUse() == WALLET_NWC) {
    nwc_getBalance(); // keeps the previous balance if it fails or times out
  } // else do nothing
}

void fetchPaymentsAsync() {
  nrOfPreviousPayments = max(nrOfPayments, 0);
  for (int i = 0; i < nrOfPreviousPayments; i++) previousPayments[i] = payments[i];
  fetchedAllPayments = false;
  nrOfPayments = 0;
  if (walletToUse() == WALLET_LNBITS) {
    fetchLNURLPayments(MAX_PAYMENTS); // nrOfPayments will be set while fetching
  } else if (walletToUse() == WALLET_NWC) {
    fetchNWCPayments(MAX_PAYMENTS);
  } // else do nothing
}

// A wallet's incoming payments don't disappear, so getting none after having some
// means the fetch failed (no wifi, a server error or a timeout). Then keep the previous
// ones on the display, and fetch them again at the next refresh.
void keepPreviousPaymentsIfFetchFailed() {
  paymentsFetchFailed = (nrOfPayments <= 0 && nrOfPreviousPayments > 0);
  if (!paymentsFetchFailed) return;
  Serial.println("WARNING: got no payments, keeping the " + String(nrOfPreviousPayments) + " previous ones on the display.");
  for (int i = 0; i < nrOfPreviousPayments; i++) payments[i] = previousPayments[i];
  nrOfPayments = nrOfPreviousPayments;
}

bool lastPaymentsFetchFailed() {
  return paymentsFetchFailed;
}

String getPayment(int nr) {
  if (nr >= 0 && nr < nrOfPayments) {
    return payments[nr];
  } else {
    Serial.println("WARNING: getPayment called with invalid payment nr " + String(nr));
    return "";
  }
}

void appendPayment(String detail) {
  // nrOfPayments starts at NOT_SPECIFIED (-1) until the first fetch
  // resets it — normalize so an early websocket/NWC payment can't
  // write to payments[-1].
  if (nrOfPayments < 0) nrOfPayments = 0;
  // Bounds check BEFORE the write: when the list is full, the old code
  // still assigned payments[MAX_PAYMENTS] — one past the end of the
  // array — corrupting whatever lives next on the heap. The LNBits call
  // site guards externally but the NWC path appends whatever the relay
  // returns, which may be more than requested.
  if (nrOfPayments >= MAX_PAYMENTS) {
    Serial.println("WARNING: payment list is full, not appending.");
    return;
  }
  payments[nrOfPayments] = detail;
  nrOfPayments++;
  Serial.println("After appending payment, the list contains:" + stringArrayToString(payments, nrOfPayments));
}

void prependPayment(String toadd) {
  if (nrOfPayments < 0) nrOfPayments = 0; // see appendPayment — pre-fetch state is -1
  // First move them all down one spot
  for (int i=min(nrOfPayments,MAX_PAYMENTS-1);i>0;i--) {
    Serial.println("Moving payment comment for item " + String(i-1) + " to item " + String(i));
    payments[i] = payments[i-1];
  }
  payments[0] = toadd;
  if (nrOfPayments<MAX_PAYMENTS) nrOfPayments++;
  Serial.println("After prepending payment, the list contains:" + stringArrayToString(payments, nrOfPayments));
}



int getBalance() {
  return currentBalance;
}

void setBalance(int balance) {
  currentBalance = balance;
}

bool balanceChanged() {
  return oldBalance != currentBalance;
}



int getNrOfPayments() {
  return nrOfPayments;
}

void setNrOfPayments(int n) {
  nrOfPayments = n;
}


// These are set by the async get_balance and list_transactions:

void setGetBalanceDone(bool done) {
    getBalanceIsDone = done;
}

bool getBalanceDone() {
  return getBalanceIsDone;
}

bool fetchedPaymentsDone() {
  return fetchedAllPayments;
}

void setFetchedPaymentsDone(bool done) {
  fetchedAllPayments = done;
}
