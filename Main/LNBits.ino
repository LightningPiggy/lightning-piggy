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
  DynamicJsonDocument doc(4096); // 4096 bytes is plenty for just the wallet details (id, name and balance info)

  DeserializationError error = deserializeJson(doc, line);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return NOT_SPECIFIED;
  }

  String walletName = doc["name"];

  if (walletName == "null") {
    Serial.println("ERROR: could not find wallet details on lnbits host " + String(lnbitsHost) + " with invoice/read key " + String(lnbitsInvoiceKey) + " so something's wrong! Did you make a typo?");
    return NOT_SPECIFIED;
  } else {
    Serial.print("Wallet name: " + walletName);
  }

  int walletBalance = doc["balance"];
  walletBalance = walletBalance / 1000;

  Serial.println(" contains " + String(walletBalance) + " sats");
  return walletBalance+balanceBiasInt;
}

void fetchLNURLPayments(int limit) {
  const String url = "/api/v1/payments?limit=" + String(limit);
  Serial.println("Getting payments from " + String(url));

  const String line = getEndpointData(lnbitsHost, url, true);
  Serial.println("Got payments: " + line);

  DynamicJsonDocument doc(limit * 4096); // 4KB per lnurlpayment should be enough for everyone (tm)
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
    Serial.println("WARNING: No receive code is configured and it can only be fetched for LNBits");
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
