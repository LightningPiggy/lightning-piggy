// DEBUG causes no wifi connection and only dummy data to be used,
// for faster startup and testing of display code etc.
//#define DEBUG

// The REPLACETHISBY... values get replaced with the user provided values by the Web Serial Installer for Lightning Piggy.
// But you can also replace them manually yourself here if you don't plan on using the Web Installer.

// MANDATORY:
// ==========

#define REPLACE_ssid "REPLACETHISBYWIFISSID_REPLACETHISBYWIFISSID_REPLACETHISBYWIFISSID"  // wifi SSID
#define REPLACE_lnbitsHost "REPLACETHISBYLNBITSHOST_REPLACETHISBYLNBITSHOST_REPLACETHISBYLNBITSHOST" // HOST NAME HERE E.G. legend.lnbits.com
#define REPLACE_lnbitsInvoiceKey "REPLACETHISBYLNBITSKEY_REPLACETHISBYLNBITSKEY_REPLACETHISBYLNBITSKEY" // lnbits wallet invoice key here

// OPTIONAL:
// =========
#define REPLACE_password "REPLACETHISBYWIFIKEY_REPLACETHISBYWIFIKEY_REPLACETHISBYWIFIKEY" // wifi password or leave empty for open network

#define REPLACE_lnbitsPort "REPLACETHISBYLNBITSPORT_REPLACETHISBYLNBITSPORT_REPLACETHISBYLNBITSPORT" // PORT NUMBER HERE E.G. 443

#define REPLACE_staticLNURLp "REPLACETHISBYSTATICLNURLPAYMENTSVALUESTRING_REPLACETHISBYSTATICLNURLPAYMENTSVALUESTRING_REPLACETHISBYSTATICLNURLPAYMENTSVALUESTRING" // faster (avoids an API call) and resolves ambiguity in case multiple lnurlp's are available

// If the fiat currency (btcPriceCurrencyChar) is not configured, then no fiat values are shown
// Example: USD, EUR, DKK, CHF, GBP, JPY, CNY, RMB, MYR,...
#define REPLACE_btcPriceCurrencyChar "REPLACETHISBYFIATCURRENCY_REPLACETHISBYFIATCURRENCY_REPLACETHISBYFIATCURRENCY"

// Configure the timezone here, example: Europe/Copenhagen
#define REPLACE_timezone "REPLACETHISBYTIMEZONE_REPLACETHISBYTIMEZONE_REPLACETHISBYTIMEZONE"

// Use the language codes from https://en.wikipedia.org/wiki/List_of_ISO_639_language_codes
// Optionally, add the territory string in capitals.
// Example: en for English, da for Danish, nl for Dutch, de for German
// Or: en_US for American English, da_DK for Danish (Denmark), nl_BE for Flemish, de_CH for Swiss High German, de or de_DE for German, es for Spanish etc...
#define REPLACE_localeSetting "REPLACETHISBYLOCALE_REPLACETHISBYLOCALE_REPLACETHISBYLOCALE"

#define REPLACE_thousandsSeparator "REPLACETHISBYTHOUSANDSSEPARATOR_REPLACETHISBYTHOUSANDSSEPARATOR_REPLACETHISBYTHOUSANDSSEPARATOR"

#define REPLACE_decimalSeparator "REPLACETHISBYDECIMALSEPARATOR_REPLACETHISBYDECIMALSEPARATOR_REPLACETHISBYDECIMALSEPARATOR"

// A bit of text to show before the boot slogan.
// Example: "Here's a bit of wisdom:"
#define REPLACE_bootSloganPrelude "REPLACETHISBYBOOTSLOGANPRELUDE_REPLACETHISBYBOOTSLOGANPRELUDE_REPLACETHISBYBOOTSLOGANPRELUDE"

// Whether or not to show the boot slogan. Set to "YES" if you want it:
#define REPLACE_showSloganAtBoot "REPLACETHISBYSHOWBOOTSLOGAN_REPLACETHISBYSHOWBOOTSLOGAN_REPLACETHISBYSHOWBOOTSLOGAN"

// Value to add to the balance (can also be negative)
// This can be used to account for the sats that have been moved to cold storage etc.
#define REPLACE_balanceBias "REPLACETHISBYBALANCEBIAS_REPLACETHISBYBALANCEBIAS_REPLACETHISBYBALANCEBIAS"
