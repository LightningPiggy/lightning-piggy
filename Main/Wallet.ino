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

void fetchPaymentsAsync() {
  if (walletToUse() == WALLET_LNBITS) {
    
  } else {
    
  } // else do nothing
}
