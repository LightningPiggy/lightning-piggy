// Wallet abstraction

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
  if (walletToUse() == WALLET_LNBITS) {
    int currentBalance = getWalletBalance();
    receivedWalletBalance(currentBalance);
  } else if (walletToUse() == WALLET_NWC) {
    nwc_getBalance();
  } // else do nothing
}

void fetchPaymentsAsync(int max_payments) {
  if (walletToUse() == WALLET_LNBITS) {
    fetchLNURLPayments(MAX_PAYMENTS);
    receivedPayments();
  } else if (walletToUse() == WALLET_NWC) {
    fetchNWCPayments(MAX_PAYMENTS);
  } // else do nothing
}

int getNrofPayments() {
  if (walletToUse() == WALLET_LNBITS) {
    return getNroflnurlPayments();
  } else if (walletToUse() == WALLET_NWC) {
    return getNrofNWCPayments();
  } else {
    return 0;
  }
}

String getPayment(int nr) {
  if (walletToUse() == WALLET_LNBITS) {
    return getLnurlPayment(nr);
  } else if (walletToUse() == WALLET_NWC) {
    return getNWCPayment(nr);
  } else {
    return "";
  }
}
