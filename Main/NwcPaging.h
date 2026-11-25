#ifndef NWC_PAGING_H
#define NWC_PAGING_H

// Decides what to do with each transaction that comes back from an NWC
// list_transactions request, while the piggy pages backwards through the
// wallet's incoming payments one at a time.
//
// The piggy first pages with "until" (the newest transaction older than the
// previous one). Some wallets ignore "until" and always return the newest
// transaction; LNbits' NWC provider does this. Once that is seen, the piggy
// pages with "offset" instead, which those wallets do honour.
//
// Kept free of Arduino types so it can be tested on a computer
// (see tests/host/test_nwc_paging.cpp).

#include <string.h>

enum NwcPagingAction {
  NWC_PAGING_APPEND,           // a new, older transaction: show it and continue
  NWC_PAGING_SWITCH_TO_OFFSET, // the wallet ignored "until": ask again for the same position using "offset"
  NWC_PAGING_END               // the wallet ignores "offset" as well: stop, rather than show duplicates
};

struct NwcPagingState {
  bool pageWithOffset;              // false: page with "until", true: page with "offset"
  int paymentsFetched;              // transactions appended so far in this fetch
  unsigned long long lastCreatedAt; // created_at of the last appended transaction
  char lastPaymentHash[65];         // payment_hash of the last appended transaction, "" if unknown
};

inline void nwcPagingStartFetch(NwcPagingState& state) {
  // pageWithOffset is kept: the wallet stays the same until the next reboot.
  state.paymentsFetched = 0;
  state.lastCreatedAt = 0;
  state.lastPaymentHash[0] = '\0';
}

// A payment hash is only used to recognise a repeated transaction when it is
// a real one: 64 hex characters. Wallets may leave it out for payments
// without an invoice (ecash, for example), and the JSON library then hands
// over the text "null".
inline bool nwcPagingIsPaymentHash(const char* hash) {
  if (hash == NULL || strlen(hash) != 64) return false;
  for (int i = 0; i < 64; i++) {
    char c = hash[i];
    bool hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    if (!hex) return false;
  }
  return true;
}

// The "until" and "offset" to send for the next request. until == 0 and
// offset == 0 mean "not sent".
inline void nwcPagingNextRequest(NwcPagingState& state, unsigned long long& until, int& offset) {
  // Without the previous transaction's created_at no "until" can be sent,
  // so page with "offset" instead.
  if (!state.pageWithOffset && state.paymentsFetched > 0 && state.lastCreatedAt == 0) state.pageWithOffset = true;
  if (state.pageWithOffset) {
    until = 0;
    offset = state.paymentsFetched;
  } else {
    until = state.paymentsFetched > 0 ? state.lastCreatedAt - 1 : 0;
    offset = 0;
  }
}

// requestedUntil is the "until" that was sent with the request this
// transaction answers (0 if none was sent).
inline NwcPagingAction nwcPagingOnTransaction(NwcPagingState& state, unsigned long long requestedUntil,
                                              unsigned long long createdAt, const char* paymentHash) {
  if (state.paymentsFetched > 0) {
    bool hashKnown = nwcPagingIsPaymentHash(paymentHash) && state.lastPaymentHash[0] != '\0';
    bool sameAsLast = hashKnown && strcmp(paymentHash, state.lastPaymentHash) == 0;
    bool notOlder = state.pageWithOffset ? state.lastCreatedAt > 0 && createdAt > state.lastCreatedAt
                                         : requestedUntil > 0 && createdAt > requestedUntil;
    if (sameAsLast || notOlder) {
      if (!state.pageWithOffset) {
        state.pageWithOffset = true;
        return NWC_PAGING_SWITCH_TO_OFFSET;
      }
      return NWC_PAGING_END;
    }
  }
  state.paymentsFetched++;
  state.lastCreatedAt = createdAt;
  if (nwcPagingIsPaymentHash(paymentHash)) {
    strncpy(state.lastPaymentHash, paymentHash, sizeof(state.lastPaymentHash) - 1);
    state.lastPaymentHash[sizeof(state.lastPaymentHash) - 1] = '\0';
  } else {
    state.lastPaymentHash[0] = '\0';
  }
  return NWC_PAGING_APPEND;
}

#endif
