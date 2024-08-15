
// Returns true if the value is configured, otherwise false.
bool isConfigured(const char * configName) {
  if ((strncmp(configName, NOTCONFIGURED, NOTCONFIGURED_LENGTH) == 0) || (strlen(configName) == 0)) {
    return false;
  } else {
    return true;
  }
}

int getConfigValueAsInt(char* configValue, int defaultValue) {
  int configInt = defaultValue;
  if (isConfigured(configValue)) {
    if (str2int(&configInt, (char*)configValue, 10) != STR2INT_SUCCESS) {
      Serial.println("WARNING: failed to convert config value ('" + String(configValue) + "') to integer, ignoring...");
    } else {
      Serial.println("Returning config value as int: " + String(configInt));
    }
  }
  return configInt;
}

// pad integer to 3 digits
// doesn't support negative numbers
String padInt(int number) {
  String padded = "";
  int zerosToAdd = 0;

  if (number < 0) {
    return ""; // negative not supported
  } else if (number < 10) {
    zerosToAdd = 2;
  } else if (number < 100) {
    zerosToAdd = 1;
  } // else if number < 1000 then no padding needed
  Serial.println("Padding " + String(number) + " needs " + zerosToAdd + " zeroes.");
  for (int i=0;i<zerosToAdd;i++) {
    padded = padded + "0";
  }
  return padded + String(number);
}

String formatIntWithSeparator(int numberAsInt) {
  int millions = numberAsInt / 1000000;
  int thousands = (numberAsInt % 1000000) / 1000;
  int remainder = numberAsInt % 1000;

  if (numberAsInt < 1000) {
    return String(numberAsInt);
  } else if (numberAsInt < 1000000) {
    return String(thousands) + getCurrentCurrencyThousandsSeparator() + padInt(remainder);
  } else {
    return String(millions) + getCurrentCurrencyThousandsSeparator() + padInt(thousands) + getCurrentCurrencyThousandsSeparator() + padInt(remainder);
  }
}

// Should the currency symbol be prepended (= placed before the amount) or not?
bool prependCurrencySymbol() {
  if ((strncmp(btcPriceCurrencyChar,"USD",3) == 0) ||
  (strncmp(btcPriceCurrencyChar,"EUR",3) == 0) ||
  (strncmp(btcPriceCurrencyChar,"GBP",3) == 0) ||
  (strncmp(btcPriceCurrencyChar, "JPY", 3) == 0) ||
  (strncmp(btcPriceCurrencyChar, "CNY", 3) == 0) ||
  (strncmp(btcPriceCurrencyChar, "BRL", 3) == 0) ||
  (strncmp(btcPriceCurrencyChar, "RMB", 3) == 0)) {
      return true;
  }
  return false;
}

String getCurrentCurrencyCode() {
  if (strncmp(btcPriceCurrencyChar, "USD", 3) == 0) {
      return "$";
  } else if ((strncmp(btcPriceCurrencyChar, "DKK", 3) == 0) || (strncmp(btcPriceCurrencyChar, "SEK", 3) == 0)) {
      return "kr.";
  } else if (strncmp(btcPriceCurrencyChar, "EUR", 3) == 0) {
      return "€";
  } else if (strncmp(btcPriceCurrencyChar, "CHF", 3) == 0) {
    return "Fr.";
  } else if (strncmp(btcPriceCurrencyChar, "GBP", 3) == 0) {
    return "£";
  } else if (strncmp(btcPriceCurrencyChar, "BRL", 3) == 0) {
    return "R$";
  } else if ((strncmp(btcPriceCurrencyChar, "JPY", 3) == 0) || (strncmp(btcPriceCurrencyChar, "CNY", 3) == 0) || (strncmp(btcPriceCurrencyChar, "RMB", 3) == 0)) {
    return "¥";
  } else {
    return "";
  }
}

char getCurrentCurrencyDecimalSeparator() {
   if (isConfigured(decimalSeparator)) {
    return decimalSeparator[0];
  } else {
    return defaultDecimalSeparator[0];
  }
}

char getCurrentCurrencyThousandsSeparator() {
  if (isConfigured(thousandsSeparator)) {
    return thousandsSeparator[0];
  } else {
    return defaultThousandsSeparator[0];
  }
}

// Replace dot with CurrencyDecimalSeparator
String floatToString(float number, int decimals) {
  char buffer[15];

  // Minimum width of 0 means it takes whatever width is required
  dtostrf(number, 0, decimals, buffer);

  for (int i = 0; i < strlen(buffer); i++) {
    if (buffer[i] == '.') {
      buffer[i] = getCurrentCurrencyDecimalSeparator();
      break;
    }
  }

  return String(buffer);
}

String ipToString(IPAddress ip) { // IP v4 only
  String ips;
  ips.reserve(16);
  ips = ip[0];  ips += '.';
  ips += ip[1]; ips += '.';
  ips += ip[2]; ips += '.';
  ips += ip[3];
  return ips;
}

/* Convert string s to int out.
 *
 * @param[out] out The converted int. Cannot be NULL.
 *
 * @param[in] s Input string to be converted.
 *
 *     The format is the same as strtol,
 *     except that the following are inconvertible:
 *
 *     - empty string
 *     - leading whitespace
 *     - any trailing characters that are not part of the number
 *
 *     Cannot be NULL.
 *
 * @param[in] base Base to interpret string in. Same range as strtol (2 to 36).
 *
 * @return Indicates if the operation succeeded, or why it failed.
 */
str2int_errno str2int(int *out, char *s, int base) {
    char *end;
    if (s[0] == '\0' || isspace(s[0]))
        return STR2INT_INCONVERTIBLE;
    errno = 0;
    long l = strtol(s, &end, base);
    /* Both checks are needed because INT_MAX == LONG_MAX is possible. */
    if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
        return STR2INT_OVERFLOW;
    if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
        return STR2INT_UNDERFLOW;
    if (*end != '\0')
        return STR2INT_INCONVERTIBLE;
    *out = l;
    return STR2INT_SUCCESS;
}


String stringArrayToString(String stringArray[], int nrOfItems) {
  String returnValue = "String Array with " + String(nrOfItems) + " items:\n";
  for (int i=0;i<nrOfItems;i++) {
    returnValue += "item " + String(i) + ":" + stringArray[i] + "\n";
  }
  return returnValue;
}

String paymentJsonToString(JsonObject areaElems) {
  if (areaElems["pending"]) return ""; // ignore pending payments

  // Payment always has an amount
  long long amount = areaElems["amount"]; // long long to support amounts above 999999000 millisats
  long amountSmaller = amount / 1000; // millisats to sats

  String paymentAmount(amountSmaller);
  String units = "sats";
  if (amountSmaller < 2) units = "sat";
  String paymentDetail = paymentAmount + " " + units;

  const char* comment;
  if ((areaElems["extra"]["tag"] && !areaElems["extra"]["comment"]) || !areaElems["memo"]) {
    Serial.println("Payment is lnurlp without comment, or regular payment without memo.");
    paymentDetail += "!";
  } else {
    if(areaElems["extra"]["comment"]) {
      comment = areaElems["extra"]["comment"];
      if (!comment && areaElems["extra"]["comment"][0]) comment = areaElems["extra"]["comment"][0]; // comments can also be a list
    } else { // areaElems["memo"]
      Serial.println("It's a regular non-lnurlp payment, using memo field.");
      comment = areaElems["memo"];
    }
    String paymentComment(comment);
    paymentDetail += ": " + paymentComment;
  }
  return paymentDetail;
}

String getRandomElementFromArray(String inputArray[], int numOfSlogans) {
  int randomIndex = random(0, numOfSlogans); // Generate a random index between 0 and numOfSlogans-1
  return inputArray[randomIndex];
}
