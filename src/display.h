#ifndef DISPLAY_H
#define DISPLAY_H

//////////////////////////////////////////////////////////////
// Uncomment correct color capability of your ePaper display
//////////////////////////////////////////////////////////////

// #define COLOR_TYPE BW        // black and white
// #define COLOR_TYPE 3C        // 3 colors - black, white and red/yellow
// #define COLOR_TYPE 4C        // 4 colors - black, white, red and yellow
// #define COLOR_TYPE GRAYSCALE // grayscale - 4 colors
// #define COLOR_TYPE 7C        // 7 colors

//////////////////////////////////////////////////////////////
// Uncomment for correct ePaper you have
//////////////////////////////////////////////////////////////

// BW
// #define DISPLAY_TYPE GDEW0154T8    // 152x152, 1.54"
// #define DISPLAY_TYPE GDEY027T91    // 176x264, 2.7"
// #define DISPLAY_TYPE GDEY029T94    // 128x296, 2.9"
// #define DISPLAY_TYPE GDEY029T71H   // 168x384, 2.9"
// #define DISPLAY_TYPE GDEQ031T10    // 240x320, 3.1"
// #define DISPLAY_TYPE GDEH0213BN    // 250x122, 2.13"
// #define DISPLAY_TYPE GDEQ042T81    // 400x300, 4.2"
// #define DISPLAY_TYPE GDEY0579T93   // 792x272, 5.79"
// #define DISPLAY_TYPE GDEQ0583T31   // 648x480, 5.83"
// #define DISPLAY_TYPE GDEW0583T8    // 648x480, 5.83"
// #define DISPLAY_TYPE WS75BWT7      // 800x480, 7.5"
// #define DISPLAY_TYPE GDEW075T7BW   // 800x480, 7.5"
// #define DISPLAY_TYPE GDEY075T7BW   // 800x480, 7.5"
// #define DISPLAY_TYPE GDEM102T91    // 960x640, 10.2"
// #define DISPLAY_TYPE GDEM1085T51   // 1360x480, 10.85"
// #define DISPLAY_TYPE GDEM133T91    // 960x680, 13.3"

// Grayscale
// #define DISPLAY_TYPE GDEY0154D67   // 200x200, 1.54"
// #define DISPLAY_TYPE GDEY0213B74   // 128x250, 2.13"
// #define DISPLAY_TYPE GDEW042T2_G   // 400x300, 4.2"
// #define DISPLAY_TYPE GDEY042T81    // 400x300, 4.2"
// #define DISPLAY_TYPE GDEQ0426T82   // 800x480, 4.26"
// #define DISPLAY_TYPE GDEW075T7     // 800x480, 7.5"
// #define DISPLAY_TYPE GDEY075T7     // 800x480, 7.5"

// 3C
// #define DISPLAY_TYPE GDEY0154Z90   // 200x200, 1.54"
// #define DISPLAY_TYPE WS27RBW264176 // 264x176, 2.7"
// #define DISPLAY_TYPE WS42YBW400300 // 400x300, 4.2"
// #define DISPLAY_TYPE GDEQ042Z21    // 400x300, 4.2"
// #define DISPLAY_TYPE GDEY042Z98    // 400x300, 4.2"
// #define DISPLAY_TYPE HINK_E075A01  // 640x384, 7.5"
// #define DISPLAY_TYPE GDEY0579Z93   // 792x272, 5.79"
// #define DISPLAY_TYPE GDEQ0583Z31   // 648x480, 5.83"
// #define DISPLAY_TYPE GDEY075Z08    // 800x480, 7.5"
// #define DISPLAY_TYPE GDEH075Z90    // 880x528, 7.5"
// #define DISPLAY_TYPE GDEM102Z91    // 960x640, 10.2"
// #define DISPLAY_TYPE GDEY116Z91    // 960x640, 11.6"
// #define DISPLAY_TYPE GDEY1248Z51   // 1304x984, 12.48"
// #define DISPLAY_TYPE GDEM133Z91    // 960x680, 13.3"

// 4C
// #define DISPLAY_TYPE GDEM0154F51H  // 200x200, 1.54"
// #define DISPLAY_TYPE GDEY0213F51   // 128x250, 2.13"
// #define DISPLAY_TYPE GDEY0266F51H  // 184x460, 2.66"
// #define DISPLAY_TYPE GDEY029F51H   // 168x384, 2.9"
// #define DISPLAY_TYPE WS3004YRBW    // 168x400, 3.00"
// #define DISPLAY_TYPE GDEM035F51    // 184x384, 3.5"
// #define DISPLAY_TYPE GDEM0397F81   // 800x480, 3.97"
// #define DISPLAY_TYPE GDEY0420F51   // 400x300, 4.2"
// #define DISPLAY_TYPE GDEM042F52    // 400x300, 4.2"
// #define DISPLAY_TYPE WS437YRBW     // 512x368, 4.37"
// #define DISPLAY_TYPE GDEY0579F51   // 792x272, 5.79"
// #define DISPLAY_TYPE GDEY0583F41   // 648x480, 5.83"
// #define DISPLAY_TYPE GDEM075F52    // 800x480, 7.5"
// #define DISPLAY_TYPE GDEM102F91    // 960x640, 10.2"
// #define DISPLAY_TYPE GDEY116F51    // 960x640, 11.6"
// #define DISPLAY_TYPE GDEY116F91    // 960x640, 11.6"

// 7C
// #define DISPLAY_TYPE GDEP0565D90   // 600x448, 5.65"
// #define DISPLAY_TYPE GDEY073D46    // 800x480, 7.3"
// #define DISPLAY_TYPE GDEP073E01    // 800x480, 7.3"

// ...
// More supported display classes in GxEPD2 can be found example here:
// https://github.com/ZinggJM/GxEPD2/blob/master/examples/GxEPD2_Example/GxEPD2_display_selection.h
// If you need, you can get definition from there and define your own display

#include <Arduino.h>

#include "utils.h"

///////////////////////
// COLOR_TYPE processing
///////////////////////

#ifdef COLOR_TYPE
  // Define color type constants for comparison (matches PixelPacker::DisplayFormat)
  #define CT_BW 0
  #define CT_GRAYSCALE 1
  #define CT_3C 2
  #define CT_4C 3
  #define CT_7C 4

// Create COLOR_TYPE_STRING constant
static constexpr const char COLOR_TYPE_STRING[] = XSTR(COLOR_TYPE);

  // Resolve COLOR_TYPE to COLOR_ID for comparison
  #define COLOR_ID XCAT(CT_, COLOR_TYPE)
  #if COLOR_ID == CT_BW
    #define TYPE_BW
  #elif COLOR_ID == CT_3C
    #define TYPE_3C
  #elif COLOR_ID == CT_4C
    #define TYPE_4C
  #elif COLOR_ID == CT_GRAYSCALE
    #define TYPE_GRAYSCALE
  #elif COLOR_ID == CT_7C
    #define TYPE_7C
  #else
    #pragma message("COLOR_TYPE: " XSTR(COLOR_TYPE))
    #error "COLOR_TYPE not supported!"
  #endif
#else
  #error "COLOR_TYPE not defined!"
#endif

///////////////////////
// DISPLAY_TYPE processing
///////////////////////

#ifdef DISPLAY_TYPE
  // Define display type constants for comparison
  // BW displays
  #define DT_GDEW0154T8 1
  #define DT_GDEY027T91 2
  #define DT_GDEY029T94 3
  #define DT_GDEY029T71H 4
  #define DT_GDEQ031T10 5
  #define DT_GDEH0213BN 6
  #define DT_GDEQ042T81 7
  #define DT_GDEY0579T93 8
  #define DT_GDEQ0583T31 9
  #define DT_GDEW0583T8 10
  #define DT_WS75BWT7 11
  #define DT_GDEW075T7BW 12
  #define DT_GDEY075T7BW 13
  #define DT_GDEM102T91 14
  #define DT_GDEM1085T51 15
  #define DT_GDEM133T91 16
  // Grayscale displays
  #define DT_GDEY0154D67 17
  #define DT_GDEY0213B74 18
  #define DT_GDEW042T2_G 19
  #define DT_GDEY042T81 20
  #define DT_GDEQ0426T82 21
  #define DT_GDEW075T7 22
  #define DT_GDEY075T7 23
  // 3C displays
  #define DT_GDEY0154Z90 24
  #define DT_WS27RBW264176 25
  #define DT_WS42YBW400300 26
  #define DT_GDEQ042Z21 27
  #define DT_GDEY042Z98 28
  #define DT_HINK_E075A01 29
  #define DT_GDEY0579Z93 30
  #define DT_GDEQ0583Z31 31
  #define DT_GDEW0583C64 32
  #define DT_GDEY075Z08 33
  #define DT_GDEH075Z90 34
  #define DT_GDEM102Z91 35
  #define DT_GDEY116Z91 36
  #define DT_GDEY1248Z51 37
  #define DT_GDEM133Z91 38
  // 4C displays
  #define DT_GDEM0154F51H 39
  #define DT_GDEY0213F51 40
  #define DT_GDEY0266F51H 41
  #define DT_GDEY029F51H 42
  #define DT_WS3004YRBW 43
  #define DT_GDEM035F51 44
  #define DT_GDEM0397F81 45
  #define DT_GDEY0420F51 46
  #define DT_GDEM042F52 47
  #define DT_WS437YRBW 48
  #define DT_GDEY0579F51 49
  #define DT_GDEY0583F41 50
  #define DT_GDEM075F52 51
  #define DT_GDEM102F91 52
  #define DT_GDEY116F51 53
  #define DT_GDEY116F91 54
  // 7C displays
  #define DT_GDEP0565D90 55
  #define DT_GDEY073D46 56
  #define DT_GDEP073E01 57

// Create DISPLAY_TYPE_STRING constant
static constexpr const char DISPLAY_TYPE_STRING[] = XSTR(DISPLAY_TYPE);

  // Resolve DISPLAY_TYPE to DISPLAY_ID for comparison (e.g., DISPLAY_ID == DT_GDEW075T7)
  #define DISPLAY_ID XCAT(DT_, DISPLAY_TYPE)

  // Validate DISPLAY_TYPE - DISPLAY_ID will be 0 if DT_<DISPLAY_TYPE> is not defined
  #if DISPLAY_ID < DT_GDEW0154T8 || DISPLAY_ID > DT_GDEP073E01
    #pragma message("DISPLAY_TYPE: " XSTR(DISPLAY_TYPE))
    #error "DISPLAY_TYPE not supported!"
  #endif
#else
  #error "DISPLAY_TYPE not defined!"
#endif

namespace Display
{
void init();
void powerOnAndInit();
void clear();
void setRotation(uint8_t rotation);

// Display info
uint16_t getWidth();
uint16_t getHeight();
uint16_t getResolutionX();
uint16_t getResolutionY();
inline const char *getColorType() { return COLOR_TYPE_STRING; }
inline const char *getDisplayType() { return DISPLAY_TYPE_STRING; }
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
void enableLightSleepDuringRefresh(bool enable);
void setBusyCallback(void (*callback)(const void *));

// Direct streaming functions (for row-by-row streaming mode)
bool supportsDirectStreaming();
void initDirectStreaming(bool partialRefresh = false, uint16_t maxRowCount = 0);
void writeRowsDirect(uint16_t yStart, uint16_t rowCount, const uint8_t *blackData, const uint8_t *colorData);
void finishDirectStreaming();
void refreshDisplay();

// Error screens
void showNoWiFiError(uint64_t sleepSeconds, const String &wikiUrl);
void showWiFiError(const String &hostname, const String &password, const String &urlWeb, const String &wikiUrl);
} // namespace Display

#endif // DISPLAY_H
