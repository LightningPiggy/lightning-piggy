// Host test for Main/NwcPaging.h: pages through simulated NWC wallets the way
// Main/NWC.ino does (one transaction per list_transactions request) and checks
// that every wallet behaviour gives the newest payments in order, without
// duplicates, in a bounded number of requests.
#include <stdio.h>
#include <string>
#include <vector>
#include "../../Main/NwcPaging.h"

struct Tx { unsigned long long createdAt; std::string hash; };

struct Wallet {
  std::vector<Tx> txs; // newest first
  bool honoursUntil, honoursOffset, honoursLimit;
  std::vector<Tx> list(unsigned long long until, int offset, int limit) const {
    std::vector<Tx> filtered;
    for (const Tx& t : txs)
      if (!honoursUntil || until == 0 || t.createdAt <= until) filtered.push_back(t);
    std::vector<Tx> out;
    int start = honoursOffset ? offset : 0;
    for (int i = start; i < (int)filtered.size(); i++) {
      if (honoursLimit && (int)out.size() >= limit) break;
      out.push_back(filtered[i]);
    }
    return out;
  }
};

static std::string key(const Tx& t) { return t.hash + "@" + std::to_string(t.createdAt); }

static int failures = 0;
static void check(bool ok, const char* what) {
  if (!ok) { printf("FAIL: %s\n", what); failures++; }
}

// Mirrors loop_nwc + getNextTransaction. Returns the hashes shown.
static std::vector<std::string> fetch(const Wallet& w, NwcPagingState& st, int toFetch, int& requests) {
  std::vector<std::string> shown;
  nwcPagingStartFetch(st);
  bool end = false;
  requests = 0;
  while (st.paymentsFetched < toFetch && !end && requests < 50) {
    unsigned long long until; int offset;
    nwcPagingNextRequest(st, until, offset);
    requests++;
    std::vector<Tx> reply = w.list(until, offset, 1);
    if (reply.empty()) { end = true; break; }
    for (const Tx& t : reply) {
      NwcPagingAction a = nwcPagingOnTransaction(st, until, t.createdAt, t.hash.c_str());
      if (a == NWC_PAGING_SWITCH_TO_OFFSET) break;
      if (a == NWC_PAGING_END) { end = true; break; }
      if ((int)shown.size() < toFetch) shown.push_back(key(t));
      if (st.paymentsFetched >= toFetch) break;
    }
  }
  return shown;
}

static std::vector<Tx> sampleTxs(int n, bool sameSecondPair) {
  std::vector<Tx> v;
  unsigned long long t = 1795464599ULL;
  for (int i = 0; i < n; i++) {
    char h[65]; snprintf(h, sizeof h, "%064x", 1000 + i);
    v.push_back({t, h});
    if (!(sameSecondPair && i == 1)) t -= 100 + i;
  }
  return v;
}

static std::string expectFirst(const std::vector<Tx>& txs, int n, std::vector<std::string>& out) {
  out.clear();
  for (int i = 0; i < n && i < (int)txs.size(); i++) out.push_back(key(txs[i]));
  return "";
}

static void scenario(const char* name, Wallet w, int toFetch, const std::vector<std::string>& expected,
                     bool expectOffsetMode, int maxRequests) {
  NwcPagingState st = {};
  int requests;
  std::vector<std::string> shown = fetch(w, st, toFetch, requests);
  bool ok = shown == expected;
  printf("%-55s shown=%zu requests=%d offsetMode=%d %s\n", name, shown.size(), requests, st.pageWithOffset, ok ? "ok" : "MISMATCH");
  check(ok, name);
  check(st.pageWithOffset == expectOffsetMode, name);
  check(requests <= maxRequests, name);
  // A second fetch (the periodic refresh) must give the same list.
  std::vector<std::string> again = fetch(w, st, toFetch, requests);
  check(again == expected, name);
  check(requests <= toFetch + 1, name);
}

int main() {
  std::vector<std::string> exp5, exp3, exp1;
  std::vector<Tx> txs = sampleTxs(8, false);
  expectFirst(txs, 5, exp5);
  expectFirst(txs, 3, exp3);
  expectFirst(txs, 1, exp1);

  scenario("compliant wallet stays on until", {txs, true, true, true}, 5, exp5, false, 6);
  scenario("wallet ignores until (LNbits NWC provider)", {txs, false, true, true}, 5, exp5, true, 7);
  scenario("wallet ignores until and offset: no duplicates", {txs, false, false, true}, 5, exp1, true, 4);
  scenario("wallet ignores offset only: stays on until", {txs, true, false, true}, 5, exp5, false, 6);
  scenario("wallet ignores limit, honours until", {txs, true, true, false}, 5, exp5, false, 2);
  scenario("wallet ignores limit and until", {txs, false, true, false}, 5, exp5, false, 2);

  std::vector<Tx> three = sampleTxs(3, false);
  std::vector<std::string> all3; expectFirst(three, 3, all3);
  scenario("fewer transactions than wanted, until ignored", {three, false, true, true}, 5, all3, true, 6);
  scenario("fewer transactions than wanted, compliant", {three, true, true, true}, 5, all3, false, 5);

  std::vector<Tx> none;
  scenario("empty wallet", {none, false, true, true}, 5, {}, false, 1);

  // Two payments in the same second: offset paging shows both.
  std::vector<Tx> same = sampleTxs(8, true);
  std::vector<std::string> expSame; expectFirst(same, 5, expSame);
  scenario("same-second pair, until ignored", {same, false, true, true}, 5, expSame, true, 7);

  // No payment hash from the wallet: timestamps alone still detect it.
  std::vector<Tx> noHash = txs;
  for (Tx& t : noHash) t.hash = "";
  std::vector<std::string> expNoHash; expectFirst(noHash, 5, expNoHash);
  scenario("no payment hash, until ignored", {noHash, false, true, true}, 5, expNoHash, true, 7);

  // Wallets that leave payment_hash out: the JSON library hands over "null"
  // (or "" when absent in other code paths). Timestamps alone must decide.
  std::vector<Tx> nullHash = txs;
  for (Tx& t : nullHash) t.hash = "null";
  std::vector<std::string> expNull; expectFirst(nullHash, 5, expNull);
  scenario("\"null\" payment hash, compliant wallet", {nullHash, true, true, true}, 5, expNull, false, 6);
  scenario("\"null\" payment hash, until ignored", {nullHash, false, true, true}, 5, expNull, true, 7);
  std::vector<Tx> mixedHash = txs;
  mixedHash[1].hash = "null"; mixedHash[2].hash = "null";
  std::vector<std::string> expMixed; expectFirst(mixedHash, 5, expMixed);
  scenario("two hashless payments in a row, compliant", {mixedHash, true, true, true}, 5, expMixed, false, 6);

  // Newest transaction without created_at: no "until" can be sent after it.
  std::vector<Tx> zeroTime = txs;
  zeroTime[0].createdAt = 0;
  std::vector<std::string> expZero; expectFirst(zeroTime, 5, expZero);
  scenario("newest has no created_at, compliant wallet", {zeroTime, true, true, true}, 5, expZero, true, 7);
  std::vector<Tx> zeroTimeNoHash = zeroTime;
  for (Tx& t : zeroTimeNoHash) t.hash = "null";
  std::vector<std::string> expZeroNoHash; expectFirst(zeroTimeNoHash, 5, expZeroNoHash);
  scenario("newest has no created_at, no hashes", {zeroTimeNoHash, true, true, true}, 5, expZeroNoHash, true, 7);

  printf(failures ? "%d FAILURES\n" : "all passed\n", failures);
  return failures ? 1 : 0;
}
