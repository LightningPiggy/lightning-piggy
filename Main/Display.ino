// The display is layed out as follows:
// <wallet balance> sat(s)                            QRCODE
// -------------------------------------------------- QRCODE
// <amount> sat(s): comment1                          QRCODE
// <amount> sat(s): comment2 comment2 comment2        status
// comment2                                           status
// <amount> sat(s): comment3                          status
// <fiatbalance> <currency> (<fiatprice> <currency)
#include "qrcoded.h"
#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#define TFT_WIDTH  135 // should be 122 but then the display is offset
#define TFT_HEIGHT DISPLAY_HEIGHT_213DEPG
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

class TFT_eSPI_Adapter : public Adafruit_GFX {
public:
  TFT_eSPI &tft;
  TFT_eSPI_Adapter(TFT_eSPI &display) : Adafruit_GFX(display.width(), display.height()), tft(display) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color) override { tft.drawPixel(x, y, color); }
  void setPartialWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    fillRect(x, y, w, h, GxEPD_BLACK);
    delay(750);
    fillRect(x, y, w, h, GxEPD_WHITE);
  }
  void firstPage() {}
  bool nextPage() { return false; }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) { tft.fillRect(x, y, w, h, color); }
  void drawImage(const unsigned char* bitmap, int x, int y, int w, int h, bool transparent) {
    uint16_t fgColor = TFT_WHITE, bgColor = TFT_BLACK;
    constexpr int transparency = 200;
    for (int row = 0; row < h; ++row) {
      for (int col = 0; col < w; ++col) {
        int byteIndex = (row * ((w + 7) / 8)) + (col / 8);
        int bitIndex = 7 - (col % 8);
        bool pixelOn = (bitmap[byteIndex] >> bitIndex) & 0x01;
        if ((rand() % 256) >= transparency && (pixelOn || !transparent)) {
          tft.drawPixel(y + row, x + (w - 1 - col), pixelOn ? fgColor : bgColor);
        }
      }
    }
  }
};

GxEPD2_BW<GxEPD2_213_BN, DISPLAY_HEIGHT_213DEPG> * display1 = nullptr;
GxEPD2_BW<GxEPD2_266_BN, DISPLAY_HEIGHT_266DEPG> * display2 = nullptr;
TFT_eSPI_Adapter* display3 = new TFT_eSPI_Adapter(tft);
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
constexpr int blackBackgroundVerticalMargin = 2, blackBackgroundHorizontalMargin = 2, marginFromQRcode = 5;
String lines[MAX_TEXT_LINES];
int nroflines = 0;
int displayToUse = NOT_SPECIFIED, balanceHeight, fiatHeight, startPaymentsHeight, waitForSloganReadUntil = 0;
bool forceRefreshBalanceAndPayments = false;
int xBeforeLNURLp;

void setup_display() {
  Serial.println("Reserving heap memory for text lines...");
  for (auto& line : lines) line.reserve(100);
  Serial.println("done.");
  if (runningOnQemu()) {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_WHITE);
    displayToUse = DISPLAY_TYPE_213DEPG_QEMU;
    for (int i = 0; i < displayWidth(); i += 5) tft.fillRect(i, 0, 1, displayHeight(), TFT_BLUE);
    for (int i = 0; i < displayHeight(); i += 5) tft.fillRect(0, i, displayWidth(), 1, TFT_RED);
    u8g2Fonts.begin(*display3);
  } else {
    display1 = new GxEPD2_BW<GxEPD2_213_BN, DISPLAY_HEIGHT_213DEPG>(GxEPD2_213_BN(5, 17, 16, 4));
    display1->init(115200, true, 2, false);
    long beforeTime = millis();
    display1->clearScreen();
    Serial.println("clearScreen operation took " + String(millis() - beforeTime) + "ms");
    if ((millis() - beforeTime) > 1500) {
      displayToUse = DISPLAY_TYPE_213DEPG;
      display1->setRotation(1);
      u8g2Fonts.begin(*display1);
    } else {
      display1->hibernate();
      delete display1;
      display1 = nullptr;
      display2 = new GxEPD2_BW<GxEPD2_266_BN, DISPLAY_HEIGHT_266DEPG>(GxEPD2_266_BN(5, 19, 4, 34));
      display2->init(115200, true, 2, false);
      display2->clearScreen();
      displayToUse = DISPLAY_TYPE_266DEPG;
      display2->setRotation(1);
      u8g2Fonts.begin(*display2);
    }
  }
  Serial.println("Detected display: " + String(displayToUse));
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  balanceHeight = roundEight(displayHeight() / 5) - 1;
  Serial.println("calculated balanceHeight: " + String(balanceHeight));
  fiatHeight = roundEight((displayHeight() * 4) / 5);
  Serial.println("calculated fiatHeight: " + String(fiatHeight));
}

int getDisplayToUse() { return displayToUse; }

void setPartialWindow(int x, int y, int h, int w) {
  Serial.println("setPartialWindow(" + String(x) + "," + String(y) + "," + String(h) + "," + String(w) + ")");
  if (displayToUse == DISPLAY_TYPE_213DEPG) display1->setPartialWindow(x, y, h, w);
  else if (displayToUse == DISPLAY_TYPE_266DEPG) display2->setPartialWindow(x, y, h, w);
  else if (displayToUse == DISPLAY_TYPE_213DEPG_QEMU) display3->setPartialWindow(x, y, h, w);
  else Serial.println("ERROR: no display detected!");
}

void displayFirstPage() {
  if (displayToUse == DISPLAY_TYPE_213DEPG) display1->firstPage();
  else if (displayToUse == DISPLAY_TYPE_266DEPG) display2->firstPage();
  else if (displayToUse == DISPLAY_TYPE_213DEPG_QEMU) display3->firstPage();
  else Serial.println("ERROR: no display detected!");
}

bool displayNextPage() {
#ifdef EMULATE_DISPLAY_TYPE_213DEPG
  Serial.println("_Update_Part : 775996");
#endif
  if (displayToUse == DISPLAY_TYPE_213DEPG) return display1->nextPage();
  else if (displayToUse == DISPLAY_TYPE_266DEPG) return display2->nextPage();
  else if (displayToUse == DISPLAY_TYPE_213DEPG_QEMU) return display3->nextPage();
  Serial.println("ERROR: no display detected!");
  return false;
}

void displayFillRect(int x, int y, int w, int h, int color) {
  if (displayToUse == DISPLAY_TYPE_213DEPG) display1->fillRect(x, y, w, h, color);
  else if (displayToUse == DISPLAY_TYPE_266DEPG) display2->fillRect(x, y, w, h, color);
  else if (displayToUse == DISPLAY_TYPE_213DEPG_QEMU) display3->fillRect(x, y, w, h, color);
  else Serial.println("ERROR: no display detected!");
}

void displayDrawImage(const unsigned char logo[], int posX, int posY, int sizeX, int sizeY, bool toggle) {
  if (displayToUse == DISPLAY_TYPE_213DEPG) display1->drawImage(logo, posX, posY, sizeX, sizeY, toggle);
  else if (displayToUse == DISPLAY_TYPE_266DEPG) display2->drawImage(logo, posX, posY, sizeX, sizeY, toggle);
  else if (displayToUse == DISPLAY_TYPE_213DEPG_QEMU) display3->drawImage(logo, posX, posY, sizeX, sizeY, toggle);
  else Serial.println("ERROR: no display detected!");
}

int displayHeight() {
  if (displayToUse == DISPLAY_TYPE_213DEPG || displayToUse == DISPLAY_TYPE_213DEPG_QEMU) return DISPLAY_WIDTH_213DEPG;
  else if (displayToUse == DISPLAY_TYPE_266DEPG) return DISPLAY_WIDTH_266DEPG;
  Serial.println("ERROR: no display detected!");
  return 0;
}

int displayWidth() {
  if (displayToUse == DISPLAY_TYPE_213DEPG || displayToUse == DISPLAY_TYPE_213DEPG_QEMU) return DISPLAY_HEIGHT_213DEPG;
  else if (displayToUse == DISPLAY_TYPE_266DEPG) return DISPLAY_HEIGHT_266DEPG;
  Serial.println("ERROR: no display detected!");
  return 0;
}

void setFont(int fontSize) {
  if (fontSize < 0) {
    Serial.println("ERROR: font size " + String(fontSize) + " not supported, setting min size");
    u8g2Fonts.setFont(u8g2_font_helvR08_te);
  } else if (fontSize == 0) u8g2Fonts.setFont(u8g2_font_helvR08_te);
  else if (fontSize == 1) u8g2Fonts.setFont(u8g2_font_helvR10_te);
  else if (fontSize == 2) u8g2Fonts.setFont(u8g2_font_helvR12_te);
  else if (fontSize == 3) u8g2Fonts.setFont(u8g2_font_helvR14_te);
  else if (fontSize == 4) u8g2Fonts.setFont(u8g2_font_helvR18_te);
  else if (fontSize == MAX_FONT) u8g2Fonts.setFont(u8g2_font_helvR24_tf);
  else {
    Serial.println("ERROR: font size " + String(fontSize) + " not supported, setting max size");
    u8g2Fonts.setFont(u8g2_font_helvR24_tf);
  }
}

int fitMaxText(String text, int maxWidth) {
  int maxLength = 0;
  uint16_t w = u8g2Fonts.getUTF8Width("$");
  uint16_t maxHeight = (u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent()) * 1.5;
  Serial.println("Got big character bounds: width " + String(w) + " and height " + String(maxHeight / 1.5) + " for text: $");
  while (maxLength < text.length() && w < maxWidth) {
    uint16_t h = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
    if (h >= maxHeight) break;
    w = u8g2Fonts.getUTF8Width(text.substring(0, maxLength + 2).c_str());
    ++maxLength;
  }
  return maxLength;
}

int drawLines(String stringArray[], int nrOfItems, int startX, int endX, int startY, bool invert, bool alignRight) {
  int yPos = startY;
  for (int linenr = 0; linenr < nroflines; ++linenr) {
    yPos += drawLine(lines[linenr], alignRight ? endX : startX, yPos, invert, alignRight);
  }
  return yPos;
}

int drawLine(String line, int xPos, int yPos, bool invert, bool alignRight) {
  int w = u8g2Fonts.getUTF8Width(line.c_str());
  int h = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
  Serial.println("Drawing text '" + String(line) + "' at (" + String(xPos) + "," + String(yPos) + ") with size " + String(w) + "x" + String(h));
  u8g2Fonts.setCursor(alignRight ? xPos - w : xPos, yPos + h);
  if (invert) {
    Serial.println("Filling rectangle for inverted text from (" + String(xPos) + "," + String(yPos) + ") of size " + String(w) + "x" + String(h));
    displayFillRect(xPos - blackBackgroundHorizontalMargin, yPos, w + blackBackgroundHorizontalMargin * 2, h + blackBackgroundVerticalMargin * 2, GxEPD_BLACK);
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);
    u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
  } else {
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  }
  Serial.println("Displaying line: " + line);
  u8g2Fonts.println(line);
  return h;
}

// Default arguments seem to cause compilation issues in the Arduino IDE unless you add .h files so let's do it like this:
int displayFit(String text, int startX, int startY, int endX, int endY, int fontSize) { return displayFit(text, startX, startY, endX, endY, fontSize, false); }
int displayFit(String text, int startX, int startY, int endX, int endY, int fontSize, bool invert) { return displayFit(text, startX, startY, endX, endY, fontSize, invert, false); }
int displayFit(String text, int startX, int startY, int endX, int endY, int fontSize, bool invert, bool alignRight) { return displayFit(text, startX, startY, endX, endY, fontSize, invert, alignRight, true); }

int displayFit(String text, int startX, int startY, int endX, int endY, int fontSize, bool invert, bool alignRight, bool drawIt) {
  long startTime = millis();
  Serial.println("displayFit " + text + " of length: " + String(text.length()) + " from (" + String(startX) + "," + String(startY) + ") to (" + String(endX) + "," + String(endY) + ") with max fontSize " + String(fontSize));
  if (!text.length()) {
    Serial.println("Aborting displayFit due to zero length text.");
    return startY;
  }
  feed_watchdog();
  int x = startX, y = startY, ex = min(displayWidth() - 1, endX), ey = min(displayHeight() - 1, endY);
  if (invert) {
    x += blackBackgroundHorizontalMargin;
    ex -= blackBackgroundHorizontalMargin;
    ey -= blackBackgroundVerticalMargin;
  }
  int yPos;
  if (drawIt) {
    Serial.println("Setting partial window: (" + String(startX) + "," + String(startY) + ") with size " + String(endX - startX + 1) + "x" + String(endY - startY + 1));
    setPartialWindow(startX, startY, endX - startX + 1, endY - startY + 1);
  }
  while (fontSize > 0) {
    nroflines = 0;
    setFont(fontSize);
    yPos = y;
    int textPos = 0;
    while (textPos < text.length() && nroflines < MAX_TEXT_LINES) {
      int chars = fitMaxText(text.substring(textPos), ex);
      lines[nroflines++] = text.substring(textPos, textPos + chars);
      yPos += u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
      textPos += chars;
    }
    if (yPos <= ey) break;
    --fontSize;
  }
  if (drawIt) {
    displayFirstPage();
    do {
      yPos = drawLines(lines, nroflines, x, ex, y, invert, alignRight);
    } while (displayNextPage());
  } else {
    yPos = drawLines(lines, nroflines, x, ex, y, invert, alignRight);
  }
  feed_watchdog();
  Serial.println("displayFit returning yPos = " + String(yPos) + " after runtime of " + String(millis() - startTime) + "ms.");
  return yPos;
}

void fastClearScreen() {
  setPartialWindow(0, 0, displayWidth(), displayHeight());
  displayFirstPage();
  do {
    displayFillRect(0, 0, displayWidth(), displayHeight(), GxEPD_WHITE);
  } while (displayNextPage());
}

void showLogo(const unsigned char logo[], int sizeX, int sizeY, int posX, int posY) {
  fastClearScreen();
  displayDrawImage(logo, posX, posY, sizeX, sizeY, false);
}

void displayBalance(int currentBalance) {
  int marginBelowBalance = displayToUse == DISPLAY_TYPE_266DEPG ? 0 : 2;
  startPaymentsHeight = balanceHeight + 1 + marginBelowBalance;
  setPartialWindow(0, 0, xBeforeLNURLp, startPaymentsHeight);
  displayFirstPage();
  do {
    displayFit(currentBalance == NOT_SPECIFIED ? "Unknown Balance" : formatIntWithSeparator(currentBalance) + " sats", 
               0, 0, xBeforeLNURLp - marginFromQRcode, balanceHeight - 1 + marginBelowBalance, 5, false, false, false);
    displayFillRect(0, balanceHeight + marginBelowBalance, xBeforeLNURLp - marginFromQRcode, 1, GxEPD_BLACK);
    displayFillRect(xBeforeLNURLp - marginFromQRcode, 0, marginFromQRcode, startPaymentsHeight, GxEPD_WHITE);
  } while (displayNextPage());
  showFiatValues(currentBalance, xBeforeLNURLp);
}

void displayPayments() {
  int maxX = xBeforeLNURLp - marginFromQRcode;
  int maxY = isConfigured(btcPriceCurrencyChar) ? fiatHeight : displayHeight();
  int startY = startPaymentsHeight;
  constexpr int marginAtBottom = 8;
  setPartialWindow(0, startY, maxX, maxY - startY);
  displayFirstPage();
  do {
    int yPos = startY;
    for (int i = 0; i < min(getNrOfPayments(), MAX_PAYMENTS) && yPos + marginAtBottom < maxY; ++i) {
      Serial.println("Displaying payment: " + getPayment(i));
      yPos = displayFit(getPayment(i), 0, yPos, maxX, maxY, 3, false, false, false);
    }
    displayFillRect(maxX, startY, marginFromQRcode, maxY - startY, GxEPD_WHITE);
  } while (displayNextPage());
}

void displayStatus(bool showsleep) {
  setFont(0);
  int qrPixels = displayWidth() - xBeforeLNURLp;
  Serial.println("qrPixels = " + String(qrPixels));
  setPartialWindow(xBeforeLNURLp, qrPixels, displayWidth() - xBeforeLNURLp, displayHeight() - qrPixels);
  displayFirstPage();
  do {
    int startY = qrPixels;
    double voltage = showsleep ? getBatteryVoltage() : getLastVoltage();
    if (voltage > 0) startY += drawLine("Batt:" + String(batteryVoltageToPercent(voltage)) + "%", displayWidth(), startY, false, true);
    String wifiString = showsleep ? "..zzZZZ" : "Wifi:" + (wifiConnected() ? String(strengthPercent(getStrength(5))) + "%" : "off");
    Serial.println("Displaying wifi string: " + wifiString);
    startY += drawLine(wifiString, displayWidth(), startY, false, true);
    String versionString = "v" + getShortVersion() + (isUpdateAvailable() ? " UPD!" : "");
    startY += drawLine(versionString, displayWidth(), startY, false, true);
    drawLine(getTimeFromNTP(), displayWidth(), startY, false, true);
  } while (displayNextPage());
}

bool displayVoltageWarning() {
  double voltage = getBatteryVoltage();
  if (voltage > 0 && voltage < 3.8) {
    displayFit(" ! LOW BATTERY (" + String(voltage) + "V) ! ", 0, displayHeight() - 30, displayWidth(), displayHeight() - 12, 2, true);
    waitForSloganReadUntil = millis() + 5000;
    return true;
  }
  return false;
}

void showFiatValues(int balance, int maxX) {
  if (!isConfigured(btcPriceCurrencyChar) || balance == NOT_SPECIFIED) {
    Serial.println("Not showing fiat values: " + String(!isConfigured(btcPriceCurrencyChar) ? "no fiat currency configured" : "couldn't find Bitcoin balance"));
    return;
  }
  float btcPrice = getBitcoinPriceCoingecko();
  if (btcPrice == NOT_SPECIFIED) {
    Serial.println("Not showing fiat values because couldn't find Bitcoin price.");
    return;
  }
  float balanceValue = btcPrice / 1e8 * balance;
  String toDisplay = prependCurrencySymbol() 
      ? getCurrentCurrencyCode() + " " + floatToString(balanceValue, 2) + " (" + getCurrentCurrencyCode() + " " + formatIntWithSeparator((int)btcPrice) + ")" 
      : floatToString(balanceValue, 2) + " " + getCurrentCurrencyCode() + " (" + formatIntWithSeparator((int)btcPrice) + " " + getCurrentCurrencyCode() + ")";
  Serial.println("Displaying fiat values: " + toDisplay);
  displayFit(toDisplay, 0, fiatHeight, maxX, displayHeight(), 2, true);
}

void showBootSlogan() {
  int timeToWait = isConfigured(bootSloganPrelude) ? (displayFit(String(bootSloganPrelude), 0, 0, displayWidth(), balanceHeight, 3), 1000) : 0;
  String slogan = getRandomBootSlogan();
  Serial.println("Showing boot slogan: " + slogan);
  displayFit(slogan, 0, balanceHeight + 8, displayWidth(), displayHeight(), 4);
  timeToWait += min((unsigned long)(strlen(slogan.c_str()) * 200), (unsigned long)(MAX_BOOTSLOGAN_SECONDS * 1000));
  Serial.println("Waiting " + String(timeToWait) + "ms (of max. " + String(MAX_BOOTSLOGAN_SECONDS) + "s) to allow the bootslogan to be read...");
  waitForSloganReadUntil = millis() + timeToWait;
}

bool doneWaitingForBootSlogan() { return millis() > waitForSloganReadUntil; }

void displayLNURLpQR(String qrData) {
  if (qrData.length() < 1 || qrData == "null") {
    Serial.println("INFO: not showing LNURLp QR code because no LNURLp code was found.");
    xBeforeLNURLp = displayWidth();
    return;
  }
  Serial.println("Building LNURLp QR code...");
  int qrVersion = getQRVersion(qrData);
  int pixSize = getQrCodePixelSize(qrVersion);
  Serial.println("qrVersion = " + String(qrVersion) + " and pixSize = " + String(pixSize));
  uint8_t qrcodeData[qrcode_getBufferSize(qrVersion)];
  QRCode qrcoded;
  qrcode_initText(&qrcoded, qrcodeData, qrVersion, 0, qrData.c_str());
  Serial.println("Displaying LNURLp QR code...");
  int qrSideSize = pixSize * qrcoded.size;
  int qrPosX = displayWidth() - qrSideSize, qrPosY = 0;
  Serial.println("qrSideSize = " + String(qrSideSize) + " and qrPosX,qrPosY = " + String(qrPosX) + "," + String(qrPosY));
  setPartialWindow(qrPosX, qrPosY, qrSideSize, qrSideSize);
  displayFirstPage();
  do {
    for (uint8_t y = 0; y < qrcoded.size; ++y) {
      for (uint8_t x = 0; x < qrcoded.size; ++x) {
        if (qrcode_getModule(&qrcoded, x, y)) {
          displayFillRect(qrPosX + pixSize * x, qrPosY + pixSize * y, pixSize, pixSize, GxEPD_BLACK);
        }
      }
    }
  } while (displayNextPage());
  xBeforeLNURLp = displayWidth() - roundEight(displayWidth() - qrPosX);
}

void setNextRefreshBalanceAndPayments(bool value) { forceRefreshBalanceAndPayments = value; }
bool getForceRefreshBalanceAndPayments() { return forceRefreshBalanceAndPayments; }

void updateStatusBar(String toShow) {
  displayFit(toShow, 0, displayHeight() - smallestFontHeight, displayWidth(), displayHeight(), 1);
}