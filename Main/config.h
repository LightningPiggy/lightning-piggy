// DEBUG causes no wifi connection and only dummy data to be used,
// for faster startup and testing of display code etc.
// #define DEBUG

// The REPLACETHISBY... values get replaced with the user provided values by the Web Serial Installer for Lightning Piggy.
// But you can also replace them manually yourself here if you don't plan on using the Web Installer.

// MANDATORY:
// ==========

// The logic with invoiceKey, staticLNURLp and walletID is as follows:
// - invoiceKey is mandatory (otherwise we can't check balance or payments)
// - staticLNURLp is recommended because then no need to fetch it so it's faster
// - walletID is recommended because it's need for the websocket, which then allows for instant incoming payments
// How this is handled in code:
// if the user hasn't provided a staticLNURLp then lnurlp and walletID are fetched using lnurlp/api/v1/links
// if the user has provided a staticLNURLp then that one is used and:
//    - if the user also provided a wallet ID then that one is used (no need to fetch it)
//    - if the user hasn't provided a wallet ID:
//        - the wallet ID is taken from incoming payments, if any are found
//        - if there are no incoming payments, then the wallet ID is fetched using lnurlp/api/v1/links
// wifi will now be setup via wifimanager

// OPTIONAL:
// =========

const char *staticLNURLp = "REPLACETHISBYSTATICLNURLPAYMENTSVALUESTRING_REPLACETHISBYSTATICLNURLPAYMENTSVALUESTRING_REPLACETHISBYSTATICLNURLPAYMENTSVALUESTRING"; // faster (avoids an API call) and resolves ambiguity in case multiple lnurlp's are available

// Regular configuration values
const char *checkUpdateHost = "m.lightningpiggy.com";

// If the fiat currency (btcPriceCurrencyChar) is not configured, then no fiat values are shown
// Example: USD, EUR, DKK, CHF, GBP, JPY, CNY, RMB,...
const char *btcPriceCurrencyChar = "REPLACETHISBYFIATCURRENCY_REPLACETHISBYFIATCURRENCY_REPLACETHISBYFIATCURRENCY";

// The ESP32 can't keep a time when it's off because it doesn't have a battery, so it needs to be fetched from a server
const char *timeServer = "worldtimeapi.org";
// timezone is added after this path, so it becomes: https://worldtimeapi.org/api/timezone/Europe/Copenhagen
const char *timeServerPath = "/api/timezone/";
// Configure the timezone here, example: Europe/Copenhagen
const char *timezone = "REPLACETHISBYTIMEZONE_REPLACETHISBYTIMEZONE_REPLACETHISBYTIMEZONE";

// Use the language codes from https://en.wikipedia.org/wiki/List_of_ISO_639_language_codes
// Optionally, add the territory string in capitals.
// Example: en for English, da for Danish, nl for Dutch, de for German
// Or: en_US for American English, da_DK for Danish (Denmark), nl_BE for Flemish, de_CH for Swiss High German, de or de_DE for German, es for Spanish etc...
const char *localeSetting = "REPLACETHISBYLOCALE_REPLACETHISBYLOCALE_REPLACETHISBYLOCALE";

const char *thousandsSeparator = "REPLACETHISBYTHOUSANDSSEPARATOR_REPLACETHISBYTHOUSANDSSEPARATOR_REPLACETHISBYTHOUSANDSSEPARATOR";
const char *defaultThousandsSeparator = ",";

const char *decimalSeparator = "REPLACETHISBYDECIMALSEPARATOR_REPLACETHISBYDECIMALSEPARATOR_REPLACETHISBYDECIMALSEPARATOR";
const char *defaultDecimalSeparator = ".";

// A bit of text to show before the boot slogan.
// Example: "Here's a bit of wisdom:"
const char *bootSloganPrelude = "REPLACETHISBYBOOTSLOGANPRELUDE_REPLACETHISBYBOOTSLOGANPRELUDE_REPLACETHISBYBOOTSLOGANPRELUDE";

// Whether or not to show the boot slogan. Set to "YES" if you want it:
const char *showSloganAtBoot = "REPLACETHISBYSHOWBOOTSLOGAN_REPLACETHISBYSHOWBOOTSLOGAN_REPLACETHISBYSHOWBOOTSLOGAN";

// Value to add to the balance (can also be negative)
// This can be used to account for the sats that have been moved to cold storage etc.
const char *balanceBias = "REPLACETHISBYBALANCEBIAS_REPLACETHISBYBALANCEBIAS_REPLACETHISBYBALANCEBIAS";
