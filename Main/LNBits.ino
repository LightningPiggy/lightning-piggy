#include "Constants.h"

String lnurlPayments[MAX_PAYMENTS];
int nroflnurlPayments = 0;

String cachedLNURLp = "";

int getWalletBalance()
{
  Serial.println("Getting wallet details...");
  const String url = "/api/v1/wallet";

  int balanceBiasInt = getConfigValueAsInt((char *)balanceBias, 0);

#ifdef DEBUG
  return 12345678 + balanceBiasInt;
#endif

  const String line = getEndpointData(lnbitsHost, url, true);
  Serial.println("Got wallet balance line: " + line);
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, line);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return NOT_SPECIFIED;
  }

  String walletName = doc["name"];

  if (walletName == "null")
  {
    Serial.println("ERROR: could not find wallet details on lnbits host " + String(lnbitsHost) + " with invoice/read key " + String(invoiceKey) + " so something's wrong! Did you make a typo?");
    return NOT_SPECIFIED;
  }
  else
  {
    Serial.print("Wallet name: " + walletName);
  }

  int walletBalance = doc["balance"];
  walletBalance = walletBalance / 1000;

  Serial.println(" contains " + String(walletBalance) + " sats");
  return walletBalance + balanceBiasInt;
}

void fetchLNURLPayments(int limit)
{
  const String url = "/api/v1/payments?limit=" + String(limit);
  Serial.println("Getting payments from " + String(url));

#ifdef DEBUG
  // regular testcase with long comment and special characters:
  const String line = "[{\"checking_id\":\"eae170927e1e30811cb242a47436ec46aff9fbc73409079ce37f3e888bd45f7b\",\"pending\":false,\"amount\":1000,\"fee\":0,\"memo\":\"piggytest\",\"time\":1690630482,\"bolt11\":\"lnbc10n1pjvf72jsp5w99r5sg4kqnhjl2syltkwxf2gm86p0p3mh2rm5fwahxmwr047l3spp5atshpyn7rccgz89jg2j8gdhvg6hln778xsys088r0ulg3z75taashp5gxwncgtpe3jmwprje9eyysh7ap0xe2ez8uy59s436xftc9vd0cdqxqzjccqpjrzjqdjs3alg9wmchtfs0nav7nkls58u8usv5pc742q8fkw4gf9fpykqkzahvuqq2sgqqyqqqqqqqqqqqeqqjq9qxpqysgqttwctdvcq64s5tv0qemcykhw4atv7l3nta0029z75ve35xxk03mp6q2cs5yznnwp0euchrq7tw8heg98p7xylq9cl5jmd45r55zttfcp83pzun\",\"preimage\":\"0000000000000000000000000000000000000000000000000000000000000000\",\"payment_hash\":\"eae170927e1e30811cb242a47436ec46aff9fbc73409079ce37f3e888bd45f7b\",\"expiry\":1690631082.0,\"extra\":{\"tag\":\"lnurlp\",\"link\":\"5cvU6X\",\"extra\":\"1000\",\"comment\":[\"Dein Körper € für Üeb Wärme großes, so let's see how it shows up. Woohoow! Looking great!\"],\"wh_status\":404,\"wh_success\":false,\"wh_message\":\"Not Found\",\"wh_response\":\"response\"},\"wallet_id\":\"12345678901234567890123456789012\",\"webhook\":null,\"webhook_status\":null},{\"checking_id\":\"d5c3a87176a0a89ab1b1e469cec4d0c4d747680742b73c37bbdd9b8fd81d8ec9\",\"pending\":false,\"amount\":1000,\"fee\":0,\"memo\":\"piggytest\",\"time\":1690622074,\"bolt11\":\"lnbc10n1pjvfkr6sp5rxqjqmufhp48etlsgxepwp33k2scctap3vvh63py3c5wtlvk93pspp56hp6sutk5z5f4vd3u35ua3xscnt5w6q8g2mncdammkdclkqa3myshp5gxwncgtpe3jmwprje9eyysh7ap0xe2ez8uy59s436xftc9vd0cdqxqzjccqpjrzjq0geslmtzh3zmklrmwe4v8l5fqv52y4wjs87nx9m9efxj74xrehh7rqafqqqwkqqqyqqqqlgqqqqqqgq2q9qxpqysgqflaf37unptvtzs738xalks6fz7xkh5jn0hem5xzmkgcznpcfa8fk6wtulumxmvpu7dzj440j20mvqjqjhhsr3y6td9asz7wlnh4080gq93wz75\",\"preimage\":\"0000000000000000000000000000000000000000000000000000000000000000\",\"payment_hash\":\"d5c3a87176a0a89ab1b1e469cec4d0c4d747680742b73c37bbdd9b8fd81d8ec9\",\"expiry\":1690622674.0,\"extra\":{\"tag\":\"lnurlp\",\"link\":\"5cvU6X\",\"extra\":\"1000\",\"comment\":[\"hello there! Here's some sats!\"],\"wh_status\":404,\"wh_success\":false,\"wh_message\":\"Not Found\",\"wh_response\":\"response\"},\"wallet_id\":\"12345678901234567890123456789012\",\"webhook\":null,\"webhook_status\":null}]";
#else
  const String line = getEndpointData(lnbitsHost, url, true);
#endif

  Serial.println("Got payments: " + line);

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, line);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  Serial.println("Displaying payment amounts and comments...");
  for (JsonObject areaElems : doc.as<JsonArray>())
  {
    String paymentDetail = paymentJsonToString(areaElems);
    if (paymentDetail.length() > 0 && nroflnurlPayments < MAX_PAYMENTS)
    {
      lnurlPayments[nroflnurlPayments] = paymentDetail;
      nroflnurlPayments++;
    }
  }

  Serial.println("After parsing LNURL payments, the list contains:" + stringArrayToString(lnurlPayments, nroflnurlPayments));
}

/**
 * @brief Get the first available LNURLp from the wallet
 *
 * @return lnurlp for accepting payments
 *
 */
String getLNURLp()
{
#ifdef DEBUG
  Serial.println("Mocking getLNURLp:");
  return "LNURL1DP68GURN8GHJ7MR9VAJKUEPWD3HXY6T5WVHXXMMD9AKXUATJD3CZ7DTRWE2NVKQ72L5D3";
#endif

  if (cachedLNURLp.length() > 0)
  {
    return cachedLNURLp;
  }

  Serial.println("Getting LNURLp link list...");

  // Get the first lnurlp
  String lnurlpData = getEndpointData(lnbitsHost, "/lnurlp/api/v1/links?all_wallets=false", true);
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, lnurlpData);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
  }
  String lnurlpId = doc[0]["id"];
  String lnurlp = doc[0]["lnurl"];

  Serial.println("Fetched LNURLp: " + lnurlp);
  cachedLNURLp = lnurlp;
  return lnurlp;
}

void addLNURLpayment(String toadd)
{
  // First move them all down one spot
  // This uses min(nroflnurlPayments,MAX_PAYMENTS-1) because:
  // - if nroflnurlPayments == 0 then lastSpot = 0 (because the new one will be at index 0 only)
  // - if nroflnurlPayments == 1 then lastSpot = 1 (because the new one will be at index 0 and the old one at index 1)
  // ...
  // - if nroflnurlPayments == 8 then lastSpot = 8
  // - if nroflnurlPayments == 9 then lastSpot = 9
  // - if nroflnurlPayments == 10 then lastSpot = 9 (because the new one will be at index 0 and the old ones at 1-9 and the last one will be dropped)
  // - if nroflnurlPayments == 11 then lastSpot = 9 (because the new one will be at index 0 and the old ones at 1-9 and the last 2 will be dropped)
  for (int i = min(nroflnurlPayments, MAX_PAYMENTS - 1); i > 0; i--)
  {
    Serial.println("Moving payment comment for item " + String(i - 1) + " to item " + String(i));
    lnurlPayments[i] = lnurlPayments[i - 1];
  }
  lnurlPayments[0] = toadd;
  if (nroflnurlPayments < MAX_PAYMENTS)
    nroflnurlPayments++;
  Serial.println("After adding LNURL payment, the list contains:" + stringArrayToString(lnurlPayments, nroflnurlPayments));
}

int getNroflnurlPayments()
{
  return nroflnurlPayments;
}

String getLnurlPayment(int item)
{
  return lnurlPayments[item];
}
