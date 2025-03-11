// The display is layed out as follows:
// <wallet balance> sat(s)                            QRCODE
// -------------------------------------------------- QRCODE
// <amount> sat(s): comment1                          QRCODE
// <amount> sat(s): comment2 comment2 comment2        status
// comment2                                           status
// <amount> sat(s): comment3                          status
// <fiatbalance> <currency> (<fiatprice> <currency)
//
#include "qrcoded.h"

//#define EMULATE_DISPLAY_TYPE_213DEPG 1 // Uncomment this to have the display work on the QEMU emulator

// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>

#ifdef EMULATE_DISPLAY_TYPE_213DEPG

#define TFT_WIDTH  135 // should be 122 but then the display is offset
#define TFT_HEIGHT 250
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

// Adapter class to make TFT_eSPI compatible with Adafruit_GFX
class TFT_eSPI_Adapter : public Adafruit_GFX {
public:
    TFT_eSPI &tft;
    TFT_eSPI_Adapter(TFT_eSPI &display) : Adafruit_GFX(display.width(), display.height()), tft(display) {}

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        tft.drawPixel(x, y, color);
    }

    void setPartialWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
      fillRect(x,y,w,h,GxEPD_BLACK);
      delay(750);
      fillRect(x,y,w,h,GxEPD_WHITE);
    }

    void firstPage() {
    }

    bool nextPage() {
      return false;
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
      tft.fillRect(x, y, w, h, color);
    }

    // Converts a 1-bit-per-pixel horizontal bitmap and draws it
    void drawImage(const unsigned char*& bitmap, int& x, int& y, int& w, int& h, bool& transparent) {
      uint16_t fgColor = TFT_WHITE;  // Foreground color (white by default)
      uint16_t bgColor = TFT_BLACK;  // Background color (black by default)

      // drawing an image pixel-by-pixel with the emulator is slow, so let's draw only 1/5 pixels to make it 5 times faster :-)
      int transparency = 200; // 0 = full opaque, all pixels drawn. 127 = half opaque, half transparent, half pixels drawn, 255 = full transparent, no pixels drawn

      for (int row = 0; row < h; row++) {
          for (int col = 0; col < w; col++) {
              int byteIndex = (row * ((w + 7) / 8)) + (col / 8);  // Locate byte in memory
              int bitIndex = 7 - (col % 8);  // Extract bit (MSB first)

              bool pixelOn = (bitmap[byteIndex] >> bitIndex) & 0x01;  // 1 = foreground, 0 = background

              // Apply dithering: Compare random value [0,255] with opacity
              if ((rand() % 256) >= transparency) {
                  if (pixelOn || !transparent) {
                      uint16_t color = pixelOn ? fgColor : bgColor;
                      //tft.drawPixel(x + col, y + row, color);
                      tft.drawPixel(y + row, x + (w - 1 - col), color); // X and Y are swapped, and the image is mirrored
                  }
              }
          }
      }
    }
};

TFT_eSPI_Adapter display1(tft);
TFT_eSPI_Adapter display2 = display1;

# else // ifndef EMULATE_DISPLAY_TYPE_213DEPG:

#define MAX_DISPLAY_BUFFER_SIZE 65536ul
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))
// Both display drivers are compiled in, and the right one is detected and used at runtime:
//GxEPD2_BW<GxEPD2_213_B74, MAX_HEIGHT(GxEPD2_213_B74)> display1(GxEPD2_213_B74(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));
GxEPD2_BW<GxEPD2_213_BN, MAX_HEIGHT(GxEPD2_213_BN)> display1(GxEPD2_213_BN(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));
GxEPD2_BW<GxEPD2_266_BN, MAX_HEIGHT(GxEPD2_266_BN)> display2(GxEPD2_266_BN(/*CS=*/ 5, /*DC=*/ 19, /*RST=*/ 4, /*BUSY=*/ 34));

#endif // #ifdef EMULATE_DISPLAY_TYPE_213DEPG


U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

long lastRefreshedVoltage = -UPDATE_VOLTAGE_PERIOD_MILLIS;  // this makes it update when first run

int statusAreaVoltageHeight = -1; // this value is cached after it's calculated so it can be reused later to updated only the voltage

int blackBackgroundVerticalMargin=2;
int blackBackgroundHorizontalMargin=2;

int marginFromQRcode = 5; // margin between balance and QR Code

String lines[MAX_TEXT_LINES];
int nroflines = 0;

int displayToUse = DISPLAY_TYPE_213DEPG;
int balanceHeight; // [0,balanceHeight] is the vertical zone of the balance region plus the line underneath it.
int fiatHeight; // [fiatHeight,displayHeight()] is the vertical zone of the fiat region.
int startPaymentsHeight;

int waitForSloganReadUntil = 0;
int lastBalance = NOT_SPECIFIED;

bool forceRefreshBalanceAndPayments = false; // should the payments list be refreshed, even if it didn't change?

int xBeforeLNURLp; // every time the QR code is shown, we store the x before it, to use for other display stuff

/* Detecting the display works by timing the clearScreen operation.
09:08:03.074 -> init operation took 23ms
09:08:06.103 -> rotation operation took 0ms
09:08:11.426 -> _Update_Full : 2290999
09:08:11.426 -> clearScreen operation took 2339ms => right display
09:08:14.452 -> init operation took 23ms
09:08:17.446 -> rotation operation took 0ms
09:08:20.507 -> _PowerOn : 3
09:08:20.507 -> _Update_Full : 1
09:08:20.507 -> clearScreen operation took 60ms => wrong display
 */
void setup_display() {

  Serial.println("Reserving heap memory for text lines...");
  for (int i = 0; i < MAX_TEXT_LINES; i++) {
    lines[i].reserve(100);  // Adjust size as needed
  }
  Serial.println("done.");
  
#ifdef EMULATE_DISPLAY_TYPE_213DEPG

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);
  for (int i=0;i<displayWidth();i=i+5) {
    tft.fillRect(i,0,1,displayHeight(),TFT_BLUE);
  }
  for (int i=0;i<displayHeight();i=i+5) {
    tft.fillRect(0,i,displayWidth(),1,TFT_RED);
  }

  u8g2Fonts.begin(display1);

# else // ifndef EMULATE_DISPLAY_TYPE_213DEPG:

  display1.init(115200, true, 2, false);
  long beforeTime = millis();
  display1.clearScreen();
  Serial.println("clearScreen operation took " + String(millis() - beforeTime) + "ms");
  if ((millis() - beforeTime) > 1500) {
    Serial.println("clearScreen took a long time so found the right display: 1!");
    display1.setRotation(1); // display is used in landscape mode
    u8g2Fonts.begin(display1); // connect u8g2 procedures to Adafruit GFX
  } else {
    display2.init(115200, true, 2, false);
    display2.clearScreen();
    displayToUse = DISPLAY_TYPE_266DEPG;
    display2.setRotation(1); // display is used in landscape mode
    u8g2Fonts.begin(display2); // connect u8g2 procedures to Adafruit GFX
  }

#endif // #ifdef EMULATE_DISPLAY_TYPE_213DEPG

  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);

  balanceHeight = roundEight(displayHeight()/5)-1;  // 0,23 inclusive is 24 pixels height.
  Serial.println("calculated balanceHeight: " + String(balanceHeight));
  fiatHeight = roundEight((displayHeight()*4)/5);
  Serial.println("calculated fiatHeight: " + String(fiatHeight));
}

int getDisplayToUse() {
  return displayToUse;
}

void setPartialWindow(int x, int y, int h, int w) {
  Serial.println("setPartialWindow(x,y,h,w) = setPartialWindow(" + String(x) + "," + String(y) + "," + String(h) + "," + String(w) + ")");
  if (displayToUse == DISPLAY_TYPE_213DEPG) {
    display1.setPartialWindow(x, y, h, w);
  } else if (displayToUse == DISPLAY_TYPE_266DEPG) {
    display2.setPartialWindow(x, y, h, w);
  } else {
    Serial.println("ERROR: there's no display to use detected!");
  }
}

void displayFirstPage() {
  if (displayToUse == DISPLAY_TYPE_213DEPG) {
    display1.firstPage();
  } else if (displayToUse == DISPLAY_TYPE_266DEPG) {
    display2.firstPage();
  } else {
    Serial.println("ERROR: there's no display to use detected!");
  }
}

bool displayNextPage() {
  if (displayToUse == DISPLAY_TYPE_213DEPG) {
    return display1.nextPage();
  } else if (displayToUse == DISPLAY_TYPE_266DEPG) {
    return display2.nextPage();
  }
  Serial.println("ERROR: there's no display to use detected!");
  return false;
}

void displayFillRect(int x, int y, int w, int h, int color) {
  if (displayToUse == DISPLAY_TYPE_213DEPG) {
    display1.fillRect(x,y,w,h,color);
  } else if (displayToUse == DISPLAY_TYPE_266DEPG) {
    display2.fillRect(x,y,w,h,color);
  } else {
    Serial.println("ERROR: there's no display to use detected!");
  }
}

void displayDrawImage(const unsigned char logo [], int posX, int posY, int sizeX, int sizeY, bool toggle) {
  if (displayToUse == DISPLAY_TYPE_213DEPG) {
    display1.drawImage(logo, posX, posY, sizeX, sizeY, toggle);
  } else if (displayToUse == DISPLAY_TYPE_266DEPG) {
    display2.drawImage(logo, posX, posY, sizeX, sizeY, toggle);
  } else {
    Serial.println("ERROR: there's no display to use detected!");
  }
}

int displayHeight() {
  if (displayToUse == DISPLAY_TYPE_213DEPG) {
    return 122;
  } else if (displayToUse == DISPLAY_TYPE_266DEPG) {
    return 152;
  }
  Serial.println("ERROR: there's no display to use detected!");
  return 0;
}

int displayWidth() {
  if (displayToUse == DISPLAY_TYPE_213DEPG) {
    return 250;
  } else if (displayToUse == DISPLAY_TYPE_266DEPG) {
    return 296;
  }
  Serial.println("ERROR: there's no display to use detected!");
  return 0;
}

// size 0 = smallest font (8pt)
// size 1 = 10pt
// size 2 = 12pt
// size 3 = 14pt
// size 4 = 18pt
// size 5 = 24pt
void setFont(int fontSize) {
  if (fontSize < 0) {
    Serial.println("ERROR: font size " + String(fontSize) + " is not supported, setting min size");
    u8g2Fonts.setFont(u8g2_font_helvR08_te);
  } else if (fontSize == 0) {
    u8g2Fonts.setFont(u8g2_font_helvR08_te);
  } else if (fontSize == 1) {
    u8g2Fonts.setFont(u8g2_font_helvR10_te);
  } else if (fontSize == 2) {
    u8g2Fonts.setFont(u8g2_font_helvR12_te);
  } else if (fontSize == 3) {
    u8g2Fonts.setFont(u8g2_font_helvR14_te);
  } else if (fontSize == 4) {
    u8g2Fonts.setFont(u8g2_font_helvR18_te);
  } else if (fontSize == MAX_FONT) {
    u8g2Fonts.setFont(u8g2_font_helvR24_tf);
  } else {
    Serial.println("ERROR: font size " + String(fontSize) + " is not supported, setting max size");
    u8g2Fonts.setFont(u8g2_font_helvR24_tf);
  }
}

// find the max length that fits the width
int fitMaxText(String text, int maxWidth) {
  //long startTime = millis();
  int maxLength = 0;
  uint16_t w, h;

  // first get height of one big character
  w = u8g2Fonts.getUTF8Width("$");
  h = u8g2Fonts.getFontAscent()-u8g2Fonts.getFontDescent();
  Serial.println("Got big character bounds:  width " +  String(w) + " and height " + String(h) + " for text: $");
  uint16_t maxHeight = h * 1.5; // ensure it's really big, but smaller than 2 lines
  //Serial.println("maxHeight = " + String(maxHeight));
  h = 0;

  while (maxLength < text.length() && h < maxHeight && w < maxWidth) {
    String textToFit = text.substring(0, maxLength+2); // end is exclusive
    w = u8g2Fonts.getUTF8Width(textToFit.c_str());
    h = u8g2Fonts.getFontAscent()-u8g2Fonts.getFontDescent();
    maxLength++;
  }

  //Serial.println("Max text length that fits: " + String(maxLength) + " calculated in " + String(millis()-startTime) + "ms.");
  return maxLength;
}

int drawLines(String stringArray[], int nrOfItems, int startX, int endX, int startY, bool invert, bool alignRight) {
  int yPos = startY;
  for (int linenr=0;linenr<nroflines;linenr++) {
    if (!alignRight) {
      yPos += drawLine(lines[linenr],startX,yPos,invert,alignRight);
    } else {
      yPos += drawLine(lines[linenr],endX,yPos,invert,alignRight);
    }
  }
  return yPos;
}

// xPos,yPos is the top left of the line (in case no alignRight) or top right of the line (in case of alignRight)
// returns line height
int drawLine(String line, int xPos, int yPos, bool invert, bool alignRight) {
  int w = u8g2Fonts.getUTF8Width(line.c_str());
  int h = u8g2Fonts.getFontAscent()-u8g2Fonts.getFontDescent();
  Serial.println("Drawing text '" + String(line) + "' at (" + String(xPos) + "," + String(yPos) + ") with size "+ String(w) + "x"+ String(h));
  if (!alignRight) {
    Serial.println("u8g2Fonts.setCursor(" + String(xPos) + "," + String(yPos + h) + ")");
    u8g2Fonts.setCursor(xPos, yPos + h); // bottom of the line
  } else {
    Serial.println("u8g2Fonts.setCursor(" + String(xPos-w) + "," + String(yPos + h) + ")");
    u8g2Fonts.setCursor(xPos-w, yPos + h); // bottom of the line
  }
  if (!invert) {
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  } else {
    Serial.println("Filling rectangle for inverted text from (" + String(xPos) + "," + String(yPos) + ") of size " + String(w) + "x" + String(h));
    displayFillRect(xPos-blackBackgroundHorizontalMargin, yPos, w+blackBackgroundHorizontalMargin*2, h+blackBackgroundVerticalMargin*2, GxEPD_BLACK);
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);
    u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
  }
  Serial.println("Displaying line: " + line);
  u8g2Fonts.println(line);
  return h;
}


int displayFit(String text, int startX, int startY, int endX, int endY, int fontSize) {
    return displayFit(text, startX, startY, endX, endY, fontSize, false);
}

int displayFit(String text, int startX, int startY, int endX, int endY, int fontSize, bool invert) {
    return displayFit(text, startX, startY, endX, endY, fontSize, invert, false);
}

int displayFit(String text, int startX, int startY, int endX, int endY, int fontSize, bool invert, bool alignRight) {
    return displayFit(text, startX, startY, endX, endY, fontSize, invert, alignRight, true);
}

// Try to fit a String into a rectangle, including the borders.
// bool bold == true means black background, white text
// returns: the y position after fitting the text
int displayFit(String text, int startXbig, int startYbig, int endXbig, int endYbig, int fontSize, bool invert, bool alignRight, bool drawIt) {
  long startTime = millis();
  bool debugDisplayFit = false;

  feed_watchdog(); // before this long-running and potentially hanging operation, it's a good time to feed the watchdog

  Serial.println("displayFit " + text + " of length: " + String(text.length()) + " from (" + String(startXbig) + "," + String(startYbig) + ") to (" + String(endXbig) + "," + String(endYbig) + ") with max fontSize " + String(fontSize));

  if (text.length() == 0) {
    Serial.println("Aborting displayFit due to zero length text.");
    return startYbig;
  }

  uint16_t h;
  int startX = startXbig;
  int startY = startYbig;
  int endX = endXbig;
  int endY = endYbig;
  int invertOffsetXbefore = blackBackgroundHorizontalMargin;
  int invertOffsetYbefore = 0;
  int invertOffsetXafter = blackBackgroundHorizontalMargin;
  int invertOffsetYafter = blackBackgroundVerticalMargin;
  if (invert) {
    // black rectangle is slightly bigger than the text; from (-2,-1) inclusive until (+2,+2) inclusive
    startX = startXbig + invertOffsetXbefore;
    startY = startYbig + invertOffsetYbefore;
    endX = endXbig - invertOffsetXafter;
    endY = endYbig - invertOffsetYafter;
  }

  // Don't go past the end of the display and remember pixels start from zero, so [0,max-1]
  endX = min(displayWidth()-1,endX);
  endY = min(displayHeight()-1,endY);

  int yPos;

  if (drawIt) {
    Serial.println("Setting partial window: (" + String(startXbig) + "," + String(startYbig) + ") with size " + String(endXbig-startXbig+1) + "x" + String(endYbig-startYbig+1));
    setPartialWindow(startXbig, startYbig, endXbig-startXbig+1, endYbig-startYbig+1);
  }
  while (fontSize > 0) {
    nroflines = 0;
    setFont(fontSize);

    yPos = startY;
    int textPos = 0;
    while (textPos < text.length() && nroflines < MAX_TEXT_LINES) {
      // Try to fit everything that still needs displaying:
      String textWithoutAlreadyPrintedPart = text.substring(textPos);
      int chars = fitMaxText(textWithoutAlreadyPrintedPart, endX);

      String textLine = text.substring(textPos, textPos+chars);
      if (debugDisplayFit) Serial.println("first line that fits: " + textLine);
      lines[nroflines] = textLine;
      nroflines++;

      h = u8g2Fonts.getFontAscent()-u8g2Fonts.getFontDescent();

      textPos += chars;
      yPos += h;
    }
    if (debugDisplayFit) Serial.println("After simulating the text, yPos = " + String(yPos) + " while endY = " + String(endY));

    // Check if the entire text fit:
    if (yPos <= endY) {
      if (debugDisplayFit) Serial.println("yPos (" + String(yPos) + ") <= endY (" + String(endY) + ") so fontSize " + String(fontSize) + " fits!");
      break; // exit the fontSize loop because it fits
    } else {
      if (debugDisplayFit) Serial.println("fontSize " + String(fontSize) + " did not fit so trying smaller...");
      fontSize--;
    }
  }

  if (drawIt) {
    // finally print the array
    displayFirstPage();
    do {
      yPos = drawLines(lines, nroflines, startX, endX, startY, invert, alignRight);
    } while (displayNextPage());
  } else {
    yPos = drawLines(lines, nroflines, startX, endX, startY, invert, alignRight);
  }
  if (debugDisplayFit) Serial.println("After writing the text, yPos = " + String(yPos) + " while endY = " + String(endY));

  feed_watchdog(); // after this long-running and potentially hanging operation, it's a good time to feed the watchdog
  if (debugDisplayFit) Serial.println("displayFit returning yPos = " + String(yPos) + " after runtime of " + String(millis() - startTime) + "ms."); // takes around 1700ms
  return yPos;
}

void fastClearScreen() {
  // display.clearScreen(); // slow
  setPartialWindow(0, 0, displayWidth(), displayHeight()); // this clear the display
  displayFirstPage();
  do {
    displayFillRect(0, 0, displayWidth(), displayHeight(), GxEPD_WHITE);
  } while (displayNextPage());
}

void showLogo(const unsigned char logo [], int sizeX, int sizeY, int posX, int posY) {
  fastClearScreen();
  displayDrawImage(logo, posX, posY, sizeX, sizeY, false);
}

void displayBalance(int currentBalance) {
  int marginBelowBalance = 2;
  if (displayToUse == 2) marginBelowBalance = 0;
  startPaymentsHeight = balanceHeight+1+marginBelowBalance;

  // Display balance from 0 to balanceHeight
  setPartialWindow(0, 0, xBeforeLNURLp, startPaymentsHeight);
  displayFirstPage();
  do {
    if (currentBalance == NOT_SPECIFIED) {
      displayFit("Unknown Balance", 0, 0, xBeforeLNURLp-marginFromQRcode, balanceHeight-1+marginBelowBalance, 5, false, false, false); // no fontdecent so all the way down to balanceHeight-1
    } else {
      displayFit(formatIntWithSeparator(currentBalance) + " sats", 0, 0, xBeforeLNURLp-marginFromQRcode, balanceHeight-1+marginBelowBalance, 5, false, false, false); // no fontdecent so all the way down to balanceHeight-1
    }
    displayFillRect(0, balanceHeight+marginBelowBalance, xBeforeLNURLp-marginFromQRcode, 1, GxEPD_BLACK); // black line
    displayFillRect(xBeforeLNURLp-marginFromQRcode, 0, marginFromQRcode, startPaymentsHeight, GxEPD_WHITE); // white margin between balance and QR code
  } while (displayNextPage());

  // Display fiat values
  showFiatValues(currentBalance, xBeforeLNURLp);
}

void displayPayments() {
  int maxX = xBeforeLNURLp - marginFromQRcode;
  int maxY = isConfigured(btcPriceCurrencyChar) ? fiatHeight : displayHeight();

  int startY = startPaymentsHeight;

  int marginAtBottom = 8;
  int w = maxX - 0;
  int h = maxY - startY;
  setPartialWindow(0, startY, w, h);
  displayFirstPage();
  do {
    int yPos = startY;
    for (int i=0;i<min(getNrOfPayments(),MAX_PAYMENTS) && yPos+marginAtBottom < maxY;i++) {
      Serial.println("Displaying payment: " + getPayment(i));
      yPos = displayFit(getPayment(i), 0, yPos, maxX, maxY, 3, false, false, false);
    }
    displayFillRect(maxX, startY, marginFromQRcode, h, GxEPD_WHITE); // margin between balance and QR code
  } while (displayNextPage());
}

void displayWifiStrengthBottom() {
  displayWifiStrength(displayHeight()-smallestFontHeight);
}

void displayWifiStrength(int y) {
  int wifiStrengthPercent = strengthPercent(getStrength(5));
  String wifiString = "Wifi:" + String(wifiStrengthPercent) + "%";
  displayFit(wifiString, displayWidth()-8*7, y, displayWidth(), displayHeight(), 1, false, true);
}

// returns the y value after showing all the status info
void displayStatus(bool showsleep) {
  setFont(0);
  int qrPixels = displayWidth() - xBeforeLNURLp; // square
  Serial.println("qrPixels = " + String(qrPixels));

  setPartialWindow(xBeforeLNURLp, qrPixels, displayWidth()-xBeforeLNURLp, displayHeight()-qrPixels);
  displayFirstPage();
  do {
    int startY = qrPixels;

    // Show battery voltage if battery detected (by heuristic)
    double voltage = getLastVoltage();
    if (showsleep) voltage = getBatteryVoltage(); // only refresh voltage before going to sleep
    if (voltage > 0) startY += drawLine("Batt:" + String(batteryVoltageToPercent(voltage)) + "%", displayWidth(), startY, false, true);

    // wifi strength or zzzz
    String wifiString = "..zzZZZ";
    if (!showsleep) {
      wifiString = "Wifi:";
      if (wifiConnected()) {
        int wifiStrengthPercent = strengthPercent(getStrength(5));
        wifiString += String(wifiStrengthPercent) + "%";
      } else {
        wifiString += "off";
      }
    }
    Serial.println("Displaying wifi string: " + wifiString);
    startY += drawLine(wifiString, displayWidth(), startY, false, true);

    String versionString = "v" + getShortVersion();
    if (isUpdateAvailable()) versionString += " UPD!";
    startY += drawLine(versionString, displayWidth(), startY, false, true);

    // Excluded because not really necessary and takes up screen space:
    //String displayString = getShortHardwareInfo();
    //startY += drawLine(displayString, displayWidth(), startY, false, true);

    // Time is only shown before sleep
    if (showsleep) {
      String currentTime = getTimeFromNTP();
      drawLine(currentTime, displayWidth(), startY, false, true);    // 6 characters, width of 8
    }
  } while (displayNextPage());
}

// returns true if voltage is low, false otherwise
bool displayVoltageWarning() {
    double voltage = getBatteryVoltage();
    // Print big fat warning on top of everything if low battery
    if (voltage > 0 && voltage < 3.8) {
      String lowBatString = " ! LOW BATTERY (" + String(voltage) + "V) ! ";
      displayFit(lowBatString, 0, displayHeight()-12-18, displayWidth(), displayHeight()-12,2, true);
      waitForSloganReadUntil = millis() + 5000;
      return true;
    } else {
      return false;
    }
}

// This shows something like:
// 123.23$ (51234.1$)
// 123.23KR (512021)
// 123.23E (51234.1E)
void showFiatValues(int balance, int maxX) {
  if (!isConfigured(btcPriceCurrencyChar)) {
    Serial.println("Not showing fiat values because no fiat currency is configured.");
    return;
  } else if (balance == NOT_SPECIFIED) {
    Serial.println("Not showing fiat balance because couldn't find Bitcoin balance.");
    return;
  }

  float btcPrice = getBitcoinPriceCoingecko();
  if (btcPrice == NOT_SPECIFIED) {
    Serial.println("Not showing fiat values because couldn't find Bitcoin price.");
    return;
  }

  // Try to add the fiat balance
  float balanceValue = btcPrice / 100000000 * balance;
  String toDisplay = prependCurrencySymbol() ? getCurrentCurrencyCode() + " " + floatToString(balanceValue, 2) + " (" + getCurrentCurrencyCode() + " " + formatIntWithSeparator((int)btcPrice) + ")" : floatToString(balanceValue, 2) + " " + getCurrentCurrencyCode() + " (" + formatIntWithSeparator((int)btcPrice) + " " + getCurrentCurrencyCode() + ")";

  Serial.println("Displaying fiat values: " + toDisplay);
  displayFit(toDisplay, 0, fiatHeight, maxX, displayHeight(), 2, true);
}


void showBootSlogan() {
  int timeToWait = 0;

  if (isConfigured(bootSloganPrelude)) {
    displayFit(String(bootSloganPrelude), 0, 0, displayWidth(), balanceHeight, 3);
    timeToWait = 1000; // since the prelude is always the same, there's no need to wait a long time to allow reading it
  }

  String slogan = getRandomBootSlogan();
  Serial.println("Showing boot slogan: " + slogan);
  displayFit(slogan, 0, balanceHeight+8, displayWidth(), displayHeight(), 4); // leave multiple of 8px margin for font descent

  // Assuming a 7 year old averages one 4-letter word per second, that's 5 characters per second.
  timeToWait += strlen(slogan.c_str()) * 1000 / 5;
  // Limit to a maximum
  timeToWait = min(timeToWait, MAX_BOOTSLOGAN_SECONDS*1000);
  Serial.println("Waiting " + String(timeToWait) + "ms (of max. " + String(MAX_BOOTSLOGAN_SECONDS) + "s) to allow the bootslogan to be read...");
  waitForSloganReadUntil = millis() + timeToWait;
}

bool doneWaitingForBootSlogan() {
  return (millis() > waitForSloganReadUntil);
}

void displayLNURLpQR(String qrData) {
  if (qrData.length() < 1 || qrData == "null") {
    Serial.println("INFO: not showing LNURLp QR code because no LNURLp code was found.");
    xBeforeLNURLp = displayWidth();
  }
  Serial.println("Building LNURLp QR code...");

  int qrVersion = getQRVersion(qrData);
  int pixSize = getQrCodePixelSize(qrVersion);
  Serial.println("qrVersion = " + String(qrVersion) + " and pixSize = " + String(pixSize));
  uint8_t qrcodeData[qrcode_getBufferSize(qrVersion)];

  QRCode qrcoded;
  const char *qrDataChar = qrData.c_str();
  qrcode_initText(&qrcoded, qrcodeData, qrVersion, 0, qrDataChar);

  Serial.println("Displaying LNURLp QR code...");
  int qrSideSize = pixSize * qrcoded.size;
  int qrPosX = displayWidth() - qrSideSize;
  int qrPosY = 0;
  Serial.println("qrSideSize = " + String(qrSideSize) + " and qrPosX,qrPosY = " + String(qrPosX) + "," + String(qrPosY));

  setPartialWindow(qrPosX, qrPosY, qrSideSize, qrSideSize);
  displayFirstPage();
  do {
    for (uint8_t y = 0; y < qrcoded.size; y++)
    {
      for (uint8_t x = 0; x < qrcoded.size; x++)
      {
        if (qrcode_getModule(&qrcoded, x, y))
        {
          displayFillRect(qrPosX + pixSize * x, qrPosY + pixSize * y, pixSize, pixSize, GxEPD_BLACK);
        }
      }
    }
  } while (displayNextPage());

  xBeforeLNURLp = qrPosX;
  xBeforeLNURLp = displayWidth()-roundEight(displayWidth()-xBeforeLNURLp);
}

void setNextRefreshBalanceAndPayments(bool value) {
  forceRefreshBalanceAndPayments = value;
}

bool getForceRefreshBalanceAndPayments() {
  return forceRefreshBalanceAndPayments;
}
