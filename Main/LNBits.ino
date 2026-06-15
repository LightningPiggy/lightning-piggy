#include "Constants.h"

String cachedLNURLp = "";

bool canUseLNBits() {
  return isConfigured(lnbitsHost) && isConfigured(lnbitsInvoiceKey);
}

int getWalletBalance() {
  Serial.println("Getting wallet details...");
  const String url = "/api/v1/wallet";

  int balanceBiasInt = getConfigValueAsInt((char*)balanceBias, 0);

  const String line = getEndpointData(lnbitsHost, url, true);
  Serial.println("Got wallet balance line: " + line);
  DynamicJsonDocument doc(2048); // 2KB is plenty for just the wallet details (id, name and balance info)

  DeserializationError error = deserializeJson(doc, line);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return NOT_SPECIFIED;
  }

  String walletName = doc["name"];

  if (walletName == "null") {
    Serial.println("ERROR: could not find wallet details on lnbits host " + String(lnbitsHost) + " with the configured invoice/read key (not printed) so something's wrong! Did you make a typo?");
    return NOT_SPECIFIED;
  } else {
    Serial.print("Wallet name: " + walletName);
  }

  // Balance is in MILLISATS — an int overflows at 2^31 msat which is only
  // ~2.15 million sats (~0.0215 BTC), turning bigger balances negative.
  // Parse as long long (the same trick paymentJsonToString already uses
  // for amounts) and convert to sats before going back to int.
  long long walletBalanceMsat = doc["balance"];
  int walletBalance = (int)(walletBalanceMsat / 1000);

  Serial.println(" contains " + String(walletBalance) + " sats");
  return walletBalance+balanceBiasInt;
}

void fetchLNURLPayments(int limit) {
  const String url = "/api/v1/payments?limit=" + String(limit);
  Serial.println("Getting payments from " + String(url));

  const String line = getEndpointData(lnbitsHost, url, true);
  Serial.println("Got payments: " + line);

  DynamicJsonDocument doc(limit * 2048); // 2KB per lnurlpayment should be enough for everyone (tm) - I measured it at 1.1KB
  DeserializationError error = deserializeJson(doc, line);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  Serial.println("Displaying payment amounts and comments...");
  for (JsonObject areaElems : doc.as<JsonArray>()) {
    String paymentDetail = paymentJsonToString(areaElems);
    if (paymentDetail.length() > 0 && getNrOfPayments()<MAX_PAYMENTS) {
      appendPayment(paymentDetail);
    }
  }
  setFetchedPaymentsDone(true);
}


/**
 * @brief Get the first available LNURLp from the wallet
 *
 * @return lnurlp for accepting payments
 *
 */
String getLNURLp() {
  // Only fetch the first one using the API if staticLNURLp was configured
  if (isConfigured(staticLNURLp)) return staticLNURLp;

  if (cachedLNURLp.length() > 0) return cachedLNURLp;

  if (walletToUse() != WALLET_LNBITS) {
    // NWC URLs can carry the wallet's Lightning Address as a lud16 query
    // parameter (coinos, LNBits nwcprovider since PR #27, ...). Use it so
    // an NWC-only piggy shows a receive QR without the user also having
    // to fill in the static receive code manually.
    if (walletToUse() == WALLET_NWC) {
      String lud16 = getLud16FromNWCURL();
      if (lud16.length() > 0) {
        Serial.println("Using lud16 from the NWC URL as receive code: " + lud16);
        cachedLNURLp = lud16;
        return lud16;
      }
    }
    Serial.println("WARNING: No receive code is configured; it can only be fetched from LNBits or taken from the NWC URL's lud16 parameter");
    return "";
  }

  // Get the first lnurlp
  Serial.println("Getting LNURLp link list...");
  String lnurlpData = getEndpointData(lnbitsHost, "/lnurlp/api/v1/links?all_wallets=false", true);
  Serial.println("Got lnurlpData: " + lnurlpData);

  JsonDocument doc; // the size of the list of links is unknown so don't skimp here
  DeserializationError error = deserializeJson(doc, lnurlpData);
  if (error) {
    Serial.print("deserializeJson() failed: "); Serial.println(error.f_str());
    return "";
  }

  String lnurlp = doc[0]["lnurl"];
  Serial.println("Fetched LNURLp: " + lnurlp);
  cachedLNURLp = lnurlp;
  return lnurlp;
}
