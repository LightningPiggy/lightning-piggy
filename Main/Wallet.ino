// Wallet abstraction

int oldBalance = NOT_SPECIFIED;
int currentBalance = NOT_SPECIFIED;

String payments[MAX_PAYMENTS];
int nrOfPayments = NOT_SPECIFIED;

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
    currentBalance = getWalletBalance();
    setGetBalanceDone(true);
  } else if (walletToUse() == WALLET_NWC) {
    nwc_getBalance();
  } // else do nothing
}

void fetchPaymentsAsync() {
  fetchedAllPayments = false;
  nrOfPayments = 0;
  if (walletToUse() == WALLET_LNBITS) {
    fetchLNURLPayments(MAX_PAYMENTS); // nrOfPayments will be set while fetching
  } else if (walletToUse() == WALLET_NWC) {
    fetchNWCPayments(MAX_PAYMENTS);
  } // else do nothing
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
  payments[nrOfPayments] = detail;
  if (nrOfPayments<MAX_PAYMENTS) nrOfPayments++;
  Serial.println("After appending payment, the list contains:" + stringArrayToString(payments, nrOfPayments));
}

void prependPayment(String toadd) {
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
