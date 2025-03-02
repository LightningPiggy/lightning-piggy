#include "qrcoded.h"

/**
 * @brief Get the size of the qr code to produce
 * 
 * @param qrData 
 * @return int 
 */
#include <Arduino.h>

// QR Code version capacity table for ECC_LOW
const int qr_capacity_numeric[40] = {
    41, 77, 127, 187, 255, 322, 370, 461, 552, 652,
    772, 883, 1022, 1101, 1250, 1408, 1548, 1725, 1903, 2061,
    2232, 2409, 2620, 2812, 3057, 3283, 3517, 3669, 3909, 4158,
    4417, 4686, 4965, 5253, 5529, 5836, 6153, 6479, 6743, 7089
};

const int qr_capacity_alphanumeric[40] = {
    25, 47, 77, 114, 154, 195, 224, 279, 335, 395,
    468, 535, 619, 667, 758, 854, 938, 1046, 1153, 1249,
    1352, 1460, 1588, 1704, 1853, 1990, 2132, 2223, 2369, 2520,
    2677, 2840, 3009, 3183, 3351, 3537, 3729, 3927, 4087, 4296
};

const int qr_capacity_byte[40] = {
    17, 32, 53, 78, 106, 134, 154, 192, 230, 271,
    321, 367, 425, 458, 520, 586, 644, 718, 792, 858,
    929, 1003, 1091, 1171, 1273, 1367, 1465, 1528, 1628, 1732,
    1840, 1952, 2068, 2188, 2303, 2431, 2563, 2699, 2809, 2953
};

// Function to determine QR mode
char getQRMode(const String &input) {
    bool isNumeric = true, isAlphanumeric = true;

    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        if (!(c >= '0' && c <= '9')) {
            isNumeric = false;
            if (!( (c >= 'A' && c <= 'Z') || strchr(" $%*+-./:", c) )) {
                isAlphanumeric = false;
                break;
            }
        }
    }

    if (isNumeric) return 'N';
    if (isAlphanumeric) return 'A';
    return 'B'; // Byte mode
}

int getQRVersion(const String &input) {
    int length = input.length();
    char mode = getQRMode(input);
    const int *capacity_table;

    switch (mode) {
        case 'N': capacity_table = qr_capacity_numeric; break;
        case 'A': capacity_table = qr_capacity_alphanumeric; break;
        default:  capacity_table = qr_capacity_byte; break;
    }

    for (int version = 1; version <= 40; version++) {
        if (length <= capacity_table[version - 1]) {
            return version;
        }
    }
    return -1; // Input too long for QR Code
}

/**
 * @brief Get the Qr Code Pixel Size object
 * 
 * @param qrCodeVersion The QR code version that is being used
 * @return int The size of the QR code pixels
 */
int getQrCodePixelSize(int qrCodeVersion) {
    Serial.println("getQrCodePixelSize for qrCodeVersion " + String(qrCodeVersion));

    if (qrCodeVersion < 1 || qrCodeVersion > 40) return -1; // Invalid QR code version

    int qrCodeHeight = 17 + 4 * qrCodeVersion; // Formula for QR code size
    int pixelHeight = floor(displayHeight() / qrCodeHeight);

    Serial.println("qrCodeHeight pixel height is: " + String(qrCodeHeight));
    Serial.println("Calced pixel height is: " + String(pixelHeight));

    if (displayWidth() > 250) return min(pixelHeight, 3);
    return min(pixelHeight, 2);
}
