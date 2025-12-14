#ifndef DISPLAY_H
#define DISPLAY_H

//////////////////////////////////////////////////////////////
// Uncomment correct color capability of your ePaper display
//////////////////////////////////////////////////////////////

// #define TYPE_BW // black and white
// #define TYPE_3C // 3 colors - black, white and red/yellow
// #define TYPE_4C // 4 colors - black, white, red and yellow
// #define TYPE_GRAYSCALE // grayscale - 4 colors
// #define TYPE_7C // 7 colors

//////////////////////////////////////////////////////////////
// Uncomment for correct ePaper you have
//////////////////////////////////////////////////////////////

// BW
// #define D_GDEW0154T8    // 152x152, 1.54"
// #define D_GDEY027T91    // 176x264, 2.7"
// #define D_GDEY029T94    // 128x296, 2.9"
// #define D_GDEY029T71H   // 168x384, 2.9"
// #define D_GDEQ031T10    // 240x320, 3.1"
// #define D_GDEQ042T81    // 400x300, 4.2"
// #define D_GDEY0579T93   // 792x272, 5.79"
// #define D_GDEQ0583T31   // 648x480, 5.83"
// #define D_WS75BWT7      // 800x480, 7.5"
// #define D_GDEW075T7BW   // 800x480, 7.5"
// #define D_GDEY075T7BW   // 800x480, 7.5"
// #define D_GDEM102T91    // 960x640, 10.2"
// #define D_GDEM1085T51   // 1360x480, 10.85"
// #define D_GDEM133T91    // 960x680, 13.3"

// Grayscale
// #define D_GDEY0154D67   // 200x200, 1.54"
// #define D_GDEY0213B74   // 128x250, 2.13"
// #define D_GDEW042T2_G   // 400x300, 4.2"
// #define D_GDEY042T81    // 400x300, 4.2"
// #define D_GDEQ0426T82   // 800x480, 4.26"
// #define D_GDEW075T7     // 800x480, 7.5"
// #define D_GDEY075T7     // 800x480, 7.5"

// 3C
// #define D_GDEY0154Z90   // 200x200, 1.54"
// #define D_WS27RBW264176 // 264x176, 2.7"
// #define D_WS42YBW400300 // 400x300, 4.2"
// #define D_GDEQ042Z21    // 400x300, 4.2"
// #define D_GDEY042Z98    // 400x300, 4.2"
// #define D_HINK_E075A01  // 640x384, 7.5"
// #define D_GDEY0579Z93   // 792x272, 5.79"
// #define D_GDEQ0583Z31   // 648x480, 5.83"
// #define D_GDEY075Z08    // 800x480, 7.5"
// #define D_GDEH075Z90    // 880x528, 7.5"
// #define D_GDEY116Z91    // 960x640, 11.6"
// #define D_GDEY1248Z51   // 1304x984, 12.48"
// #define D_GDEM133Z91    // 960x680, 13.3"

// 4C
// #define D_GDEY0213F51   // 128x250, 2.13"
// #define D_GDEY0266F51H  // 184x460, 2.66"
// #define D_GDEY029F51H   // 168x384, 2.9"
// #define D_WS3004YRBW    // 168x400, 3.00"
// #define D_GDEY0420F51   // 400x300, 4.2"
// #define D_WS437YRBW     // 512x368, 4.37"
// #define D_GDEY0579F51   // 792x272, 5.79"
// #define D_GDEM075F52    // 800x480, 7.5"
// #define D_GDEY116F51    // 960x640, 11.6"

// 7C
// #define D_GDEP0565D90   // 600x448, 5.65"
// #define D_GDEY073D46    // 800x480, 7.3"
// #define D_GDEP073E01    // 800x480, 7.3"

// ...
// More supported display classes in GxEPD2 can be found example here:
// https://github.com/ZinggJM/GxEPD2/blob/master/examples/GxEPD2_Example/GxEPD2_display_selection.h
// If you need, you can get definition from there and define your own display

#include <Arduino.h>

namespace Display
{
void init();
void clear();
void setRotation(uint8_t rotation);

// Display info
uint16_t getWidth();
uint16_t getHeight();
uint16_t getResolutionX();
uint16_t getResolutionY();
const char *getColorType();
uint16_t getNumberOfPages();

// M5Stack specific
void initM5();
void powerOffM5();

// LED/Pixel functions (for boards with RGB LEDs)
void pixelInit();
void resetPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);

// Drawing functions
void drawPixel(int16_t xCord, int16_t yCord, uint16_t color);
void drawQrCode(const char *qrStr, int qrSize, int yCord, int xCord, byte qrSizeMulti = 1);
void setTextPos(const String &text, int xCord, int yCord);
void centeredText(const String &text, int xCord, int yCord);

// Page functions
void setToFullWindow();
void setToPartialWindow(int16_t xCord, int16_t yCord, int16_t width, int16_t height);
bool supportsPartialRefresh();
void setToFirstPage();
bool setToNextPage();

// Error screens
void showNoWiFiError(uint64_t sleepMinutes, const String &wikiUrl);
void showWiFiError(const String &hostname, const String &password, const String &urlWeb, const String &wikiUrl);
} // namespace Display

#endif // DISPLAY_H
