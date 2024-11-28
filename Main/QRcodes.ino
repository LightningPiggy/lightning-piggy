#include "qrcoded.h"

/*
 * returns: x value before QR code
 */
int showLNURLpQR(String qrData)
{
  if (qrData.length() < 1 || qrData == "null")
  {
    Serial.println("INFO: not showing LNURLp QR code because no LNURLp code was found.");
    return displayWidth();
  }
  Serial.println("Building LNURLp QR code...");

  const char *qrDataChar = qrData.c_str();
  QRCode qrcoded;

  int qrVersion = getQrCodeVersion(qrData);
  int pixSize = getQrCodePixelSize(qrVersion);
  uint8_t qrcodeData[qrcode_getBufferSize(qrVersion)];

  qrcode_initText(&qrcoded, qrcodeData, qrVersion, 0, qrDataChar);

  Serial.println("Displaying LNURLp QR code...");
  int qrSideSize = pixSize * qrcoded.size;
  int qrPosX = displayWidth() - qrSideSize;
  int qrPosY = 0;

  // display.setPartialWindow(qrPosX, qrPosY, qrSideSize, qrSideSize);
  setPartialWindow(0, 0, displayWidth(), displayHeight()); // this is the first thing that gets displayed so blank the entire screen
  displayFirstPage();
  do
  {
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

  return qrPosX; // returns 192 on 250px wide display
}

/**
 * @brief Get the size of the qr code to produce
 *
 * @param qrData
 * @return int
 */
int getQrCodeVersion(String qrData)
{
  int qrVersion = 0;
  int stringLength = qrData.length();

  // Using this chart with ECC_LOW https://github.com/ricmoo/QRCode#data-capacities
  if (stringLength <= 25)
  {
    qrVersion = 1;
  }
  else if (stringLength <= 47)
  {
    qrVersion = 2;
  }
  else if (stringLength <= 77)
  {
    qrVersion = 3;
  }
  else if (stringLength <= 114)
  {
    qrVersion = 4;
  }
  else if (stringLength <= 154)
  {
    qrVersion = 5;
  }
  else if (stringLength <= 195)
  {
    qrVersion = 6;
  }
  else if (stringLength <= 367)
  {
    qrVersion = 11;
  }
  else
  {
    qrVersion = 28;
  }

  return qrVersion;
}

/**
 * @brief Get the Qr Code Pixel Size object
 *
 * @param qrCodeVersion The QR code version that is being used
 * @return int The size of the QR code pixels
 */
int getQrCodePixelSize(int qrCodeVersion)
{
  Serial.println("getQrCodePixelSize for qrCodeVersion " + String(qrCodeVersion));

  // Using https://github.com/ricmoo/QRCode#data-capacities
  // Get the QR code size (blocks not pixels)
  int qrCodeHeight = 0;
  switch (qrCodeVersion)
  {
  case 1:
    qrCodeHeight = 21;
    break;
  case 2:
    qrCodeHeight = 25;
    break;
  case 3:
    qrCodeHeight = 29;
    break;
  case 4:
    qrCodeHeight = 33;
    break;
  case 5:
    qrCodeHeight = 37;
    break;
  case 6:
    qrCodeHeight = 41;
    break;
  case 7:
    qrCodeHeight = 45;
    break;
  case 8:
    qrCodeHeight = 49;
    break;
  case 9:
    qrCodeHeight = 53;
    break;
  case 10:
    qrCodeHeight = 57;
    break;
  case 11:
    qrCodeHeight = 61;
    break;
  default:
    qrCodeHeight = 129;
    break;
  }
  int pixelHeight = floor(displayHeight() / qrCodeHeight);
  Serial.println("qrCodeHeight pixel height is: " + String(qrCodeHeight));
  Serial.println("Calced pixel height is: " + String(pixelHeight));

  if (displayWidth() > 250)
  {
    // On big displays, make the QR code 50% bigger, for esthetics.
    return min(pixelHeight, 3);
  }

  // QR codes of height 1 are still scan-able, but height 2 seems to be a safe "easy scan" value.
  // Return the minimal pixelHeight possible, to take up the least amount of space on the display:
  return min(pixelHeight, 2);
}
