#include "display.h"

#include "board.h"
#include "logger.h"

// ESP32 sleep functions for light sleep during display refresh
#include <esp_sleep.h>

// Calculate optimal page height based on buffer size, display dimensions, and bits per pixel
// Returns full height if it fits in buffer, otherwise the maximum height that fits
#define CALC_PAGE_HEIGHT(height, width, bpp)                                                                           \
  (((BOARD_MAX_PAGE_BUFFER_SIZE * 8) / ((width) * (bpp)) >= (height))                                                  \
     ? (height)                                                                                                        \
     : ((BOARD_MAX_PAGE_BUFFER_SIZE * 8) / ((width) * (bpp))))

#ifdef ES3ink
  #include <Adafruit_NeoPixel.h>
static Adafruit_NeoPixel pixel(1, RGBledPin, NEO_GRB + NEO_KHZ800);
#endif

#ifdef REMAP_SPI
  #include "SPI.h"
static SPIClass hspi(HSPI);
#endif

///////////////////////
// ePaper library includes based on type
///////////////////////

// 2 colors (Black and White)
#ifdef TYPE_BW
  #include <GxEPD2_BW.h>

// 3 colors (Black, White and Red/Yellow)
#elif defined TYPE_3C
  #include <GxEPD2_3C.h>

// 4 colors (Black, White, Red and Yellow)
#elif defined TYPE_4C
  #include <GxEPD2_4C.h>

// 4 colors (Grayscale - Black, Darkgrey, Lightgrey, White) (https://github.com/ZinggJM/GxEPD2_4G)
#elif defined TYPE_GRAYSCALE
  #include "GxEPD2_4G_4G.h"
  #include "GxEPD2_4G_BW.h"

// 7 colors
#elif defined TYPE_7C
  #include <GxEPD2_7C.h>

#endif

///////////////////////
// Display instance - selected by DISPLAY_ID
///////////////////////

///////////////////////
// BW
///////////////////////

// GDEW0154T8 - BW, 152x152px, 1.54"
#if DISPLAY_ID == DT_GDEW0154T8
GxEPD2_BW<GxEPD2_154_T8, GxEPD2_154_T8::HEIGHT> display(GxEPD2_154_T8(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEY027T91 - BW, 176x264px, 2.7"
#elif DISPLAY_ID == DT_GDEY027T91
GxEPD2_BW<GxEPD2_270_GDEY027T91, GxEPD2_270_GDEY027T91::HEIGHT> display(GxEPD2_270_GDEY027T91(PIN_SS, PIN_DC, PIN_RST,
                                                                                              PIN_BUSY));

// GDEY029T94 - BW, 128x296px, 2.9"
#elif DISPLAY_ID == DT_GDEY029T94
GxEPD2_BW<GxEPD2_290_GDEY029T94, GxEPD2_290_GDEY029T94::HEIGHT> display(GxEPD2_290_GDEY029T94(PIN_SS, PIN_DC, PIN_RST,
                                                                                              PIN_BUSY));

// GDEY029T71H - BW, 168x384px, 2.9"
#elif DISPLAY_ID == DT_GDEY029T71H
GxEPD2_BW<GxEPD2_290_GDEY029T71H, GxEPD2_290_GDEY029T71H::HEIGHT> display(GxEPD2_290_GDEY029T71H(PIN_SS, PIN_DC,
                                                                                                 PIN_RST, PIN_BUSY));

// GDEQ031T10 - BW, 240x320px, 3.1"
#elif DISPLAY_ID == DT_GDEQ031T10
GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display(GxEPD2_310_GDEQ031T10(PIN_SS, PIN_DC, PIN_RST,
                                                                                              PIN_BUSY));

// 2.13" BW, 250x122px
#elif DISPLAY_ID == DT_GDEH0213BN
GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> display(GxEPD2_213_BN(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEQ042T81 - BW, 400x300px, 4.2"
#elif DISPLAY_ID == DT_GDEQ042T81
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(PIN_SS, PIN_DC, PIN_RST,
                                                                                              PIN_BUSY));

// GDEY0579T93 - BW, 792x272px, 5.79"
#elif DISPLAY_ID == DT_GDEY0579T93
GxEPD2_BW<GxEPD2_579_GDEY0579T93, GxEPD2_579_GDEY0579T93::HEIGHT> display(GxEPD2_579_GDEY0579T93(PIN_SS, PIN_DC,
                                                                                                 PIN_RST, PIN_BUSY));

// GDEQ0583T31 - BW, 648x480px, 5.83"
#elif DISPLAY_ID == DT_GDEQ0583T31
GxEPD2_BW<GxEPD2_583_GDEQ0583T31, GxEPD2_583_GDEQ0583T31::HEIGHT> display(GxEPD2_583_GDEQ0583T31(PIN_SS, PIN_DC,
                                                                                                 PIN_RST, PIN_BUSY));

// WS75BWT7 - BW, 800x480px, 7.5"
#elif DISPLAY_ID == DT_WS75BWT7
GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEW075T7 - BW, 800x480px, 7.5"
#elif DISPLAY_ID == DT_GDEW075T7BW
GxEPD2_BW<GxEPD2_750, GxEPD2_750::HEIGHT> display(GxEPD2_750(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEY075T7BW - BW, 800x480px, 7.5"
#elif DISPLAY_ID == DT_GDEY075T7BW
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display(GxEPD2_750_GDEY075T7(PIN_SS, PIN_DC, PIN_RST,
                                                                                           PIN_BUSY));

// GDEM102T91 - BW, 960x640px, 10.2"
#elif DISPLAY_ID == DT_GDEM102T91
GxEPD2_BW<GxEPD2_1020_GDEM102T91, CALC_PAGE_HEIGHT(GxEPD2_1020_GDEM102T91::HEIGHT, GxEPD2_1020_GDEM102T91::WIDTH, 1)>
  display(GxEPD2_1020_GDEM102T91(PIN_SS, PIN_DC, PIN_ RST, PIN_BUSY));

// GDEM1085T51 - BW, 1360x480px, 10.85"
#elif DISPLAY_ID == DT_GDEM1085T51
GxEPD2_BW<GxEPD2_1085_GDEM1085T51, CALC_PAGE_HEIGHT(GxEPD2_1085_GDEM1085T51::HEIGHT, GxEPD2_1085_GDEM1085T51::WIDTH, 1)>
  display(GxEPD2_1085_GDEM1085T51(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY, PIN_CS2));

// GDEM133T91 - BW, 960x680px, 13.3"
#elif DISPLAY_ID == DT_GDEM133T91
GxEPD2_BW<GxEPD2_1330_GDEM133T91, CALC_PAGE_HEIGHT(GxEPD2_1330_GDEM133T91::HEIGHT, GxEPD2_1330_GDEM133T91::WIDTH, 1)>
  display(GxEPD2_1330_GDEM133T91(PIN_SS, PIN_DC, PIN_BUSY));

///////////////////////
// Grayscale
///////////////////////

// GDEY0154D67 - Grayscale, 200x200px, 1.54"
#elif DISPLAY_ID == DT_GDEY0154D67
GxEPD2_4G_4G<GxEPD2_154_GDEY0154D67, GxEPD2_154_GDEY0154D67::HEIGHT> display(GxEPD2_154_GDEY0154D67(PIN_SS, PIN_DC,
                                                                                                    PIN_RST, PIN_BUSY));

// GDEY0213B74 - Grayscale, 128x250px, 2.13"
#elif DISPLAY_ID == DT_GDEY0213B74
GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT> display(GxEPD2_213_GDEY0213B74(PIN_SS, PIN_DC,
                                                                                                    PIN_RST, PIN_BUSY));

// GDEW042T2_G - Grayscale, 400x300px, 4.2"
#elif DISPLAY_ID == DT_GDEW042T2_G
GxEPD2_4G_4G<GxEPD2_420, GxEPD2_420::HEIGHT> display(GxEPD2_420(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEY042T81 - Grayscale, 400x300px, 4.2"
#elif DISPLAY_ID == DT_GDEY042T81
GxEPD2_4G_4G<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(PIN_SS, PIN_DC,
                                                                                                 PIN_RST, PIN_BUSY));

// GDEQ0426T82 - Grayscale, 800x480px, 4.26"
#elif DISPLAY_ID == DT_GDEQ0426T82
GxEPD2_4G_4G<GxEPD2_426_GDEQ0426T82, CALC_PAGE_HEIGHT(GxEPD2_426_GDEQ0426T82::HEIGHT, GxEPD2_426_GDEQ0426T82::WIDTH, 2)>
  display(GxEPD2_426_GDEQ0426T82(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEY075T7 - Grayscale, 800x480px, 7.5"
#elif DISPLAY_ID == DT_GDEW075T7
GxEPD2_4G_4G<GxEPD2_750_T7, CALC_PAGE_HEIGHT(GxEPD2_750_T7::HEIGHT, GxEPD2_750_T7::WIDTH, 2)> display(
  GxEPD2_750_T7(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEY075T7 - Grayscale, 800x480px, 7.5"
#elif DISPLAY_ID == DT_GDEY075T7
GxEPD2_4G_4G<GxEPD2_750_GDEY075T7, CALC_PAGE_HEIGHT(GxEPD2_750_GDEY075T7::HEIGHT, GxEPD2_750_GDEY075T7::WIDTH, 2)>
  display(GxEPD2_750_GDEY075T7(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

///////////////////////
// 3C
///////////////////////

// GDEY0154Z90 - 3C, 200x200px, 1.54"
#elif DISPLAY_ID == DT_GDEY0154Z90
GxEPD2_3C<GxEPD2_154_Z90c, GxEPD2_154_Z90c::HEIGHT> display(GxEPD2_154_Z90c(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// D_WS27RBW264176 - 3C, 264x176px, 2.7"
#elif DISPLAY_ID == DT_WS27RBW264176
GxEPD2_3C<GxEPD2_270c, GxEPD2_270c::HEIGHT> display(GxEPD2_270c(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// WS42YBW400300 - 3C, 400x300px, 4.2"
#elif DISPLAY_ID == DT_WS42YBW400300
GxEPD2_3C<GxEPD2_420c, GxEPD2_420c::HEIGHT> display(GxEPD2_420c(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEQ042Z21 - 3C, 400x300px, 4.2"
#elif DISPLAY_ID == DT_GDEQ042Z21
GxEPD2_3C<GxEPD2_420c_Z21, GxEPD2_420c_Z21::HEIGHT> display(GxEPD2_420c_Z21(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEY042Z98 - 3C, 400x300px, 4.2"
#elif DISPLAY_ID == DT_GDEY042Z98
GxEPD2_3C<GxEPD2_420c_GDEY042Z98, GxEPD2_420c_GDEY042Z98::HEIGHT> display(GxEPD2_420c_GDEY042Z98(PIN_SS, PIN_DC,
                                                                                                 PIN_RST, PIN_BUSY));

// HINK_E075A01 - 3C, 640x384px, 7.5"
#elif DISPLAY_ID == DT_HINK_E075A01
GxEPD2_3C<GxEPD2_750c, GxEPD2_750c::HEIGHT> display(GxEPD2_750c(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEY0579Z93 - 3C, 792x272px, 5.79"
#elif DISPLAY_ID == DT_GDEY0579Z93
GxEPD2_3C<GxEPD2_579c_GDEY0579Z93, GxEPD2_579c_GDEY0579Z93::HEIGHT> display(GxEPD2_579c_GDEY0579Z93(PIN_SS, PIN_DC,
                                                                                                    PIN_RST, PIN_BUSY));

// GDEQ0583Z31 - 3C, 648x480px, 5.83"
#elif DISPLAY_ID == DT_GDEQ0583Z31
GxEPD2_3C<GxEPD2_583c_Z83, CALC_PAGE_HEIGHT(GxEPD2_583c_Z83::HEIGHT, GxEPD2_583c_Z83::WIDTH, 2)> display(
  GxEPD2_583c_Z83(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEY075Z08 - 3C, 800x480px, 7.5"
#elif DISPLAY_ID == DT_GDEY075Z08
GxEPD2_3C<GxEPD2_750c_Z08, CALC_PAGE_HEIGHT(GxEPD2_750c_Z08::HEIGHT, GxEPD2_750c_Z08::WIDTH, 2)> display(
  GxEPD2_750c_Z08(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEH075Z90 - 3C, 880x528px, 7.5"
#elif DISPLAY_ID == DT_GDEH075Z90
GxEPD2_3C<GxEPD2_750c_Z90, CALC_PAGE_HEIGHT(GxEPD2_750c_Z90::HEIGHT, GxEPD2_750c_Z90::WIDTH, 2)> display(
  GxEPD2_750c_Z90(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEY116Z91 - 3C, 960x640px, 11.6"
#elif DISPLAY_ID == DT_GDEY116Z91
GxEPD2_3C<GxEPD2_1160c_GDEY116Z91, CALC_PAGE_HEIGHT(GxEPD2_1160c_GDEY116Z91::HEIGHT, GxEPD2_1160c_GDEY116Z91::WIDTH, 2)>
  display(GxEPD2_1160c_GDEY116Z91(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEY1248Z51 - 3C, 1304x984px, 12.48"
#elif DISPLAY_ID == DT_GDEY1248Z51
GxEPD2_3C<GxEPD2_1248c, CALC_PAGE_HEIGHT(GxEPD2_1248c::HEIGHT, GxEPD2_1248c::WIDTH, 2)> display(
  GxEPD2_1248c(/*sck=*/12, /*miso=*/-1, /*mosi=*/11, /*cs_m1=*/10, /*cs_s1=*/18, /*cs_m2=*/48, /*cs_s2=*/41,
               /*dc1=*/46, /*dc2=*/45, /*rst1=*/3, /*rst2=*/39, /*busy_m1=*/8, /*busy_s1=*/17, /*busy_m2=*/40,
               /*busy_s2=*/16));

// GDEM133Z91 - 3C, 960x680px, 13.3"
#elif DISPLAY_ID == DT_GDEM133Z91
GxEPD2_3C<GxEPD2_1330c_GDEM133Z91, CALC_PAGE_HEIGHT(GxEPD2_1330c_GDEM133Z91::HEIGHT, GxEPD2_1330c_GDEM133Z91::WIDTH, 2)>
  display(GxEPD2_1330c_GDEM133Z91(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

///////////////////////
// 4C
///////////////////////

// GDEY0213F51 - 4C, 128x250px, 2.13"
#elif DISPLAY_ID == DT_GDEY0213F51
GxEPD2_4C<GxEPD2_213c_GDEY0213F51, GxEPD2_213c_GDEY0213F51::HEIGHT> display(GxEPD2_213c_GDEY0213F51(PIN_SS, PIN_DC,
                                                                                                    PIN_RST, PIN_BUSY));

// GDEY0266F51H - 4C, 184x460px, 2.66"
#elif DISPLAY_ID == DT_GDEY0266F51H
GxEPD2_4C<GxEPD2_266c_GDEY0266F51H, GxEPD2_266c_GDEY0266F51H::HEIGHT> display(GxEPD2_266c_GDEY0266F51H(PIN_SS, PIN_DC,
                                                                                                       PIN_RST,
                                                                                                       PIN_BUSY));

// GDEY029F51H - 4C, 168x384px, 2.9"
#elif DISPLAY_ID == DT_GDEY029F51H
GxEPD2_4C<GxEPD2_290c_GDEY029F51H, GxEPD2_290c_GDEY029F51H::HEIGHT> display(GxEPD2_290c_GDEY029F51H(PIN_SS, PIN_DC,
                                                                                                    PIN_RST, PIN_BUSY));

// WS3004YRBW - 4C, 168x400px, 3.00"
#elif DISPLAY_ID == DT_WS3004YRBW
GxEPD2_4C<GxEPD2_300c, GxEPD2_300c::HEIGHT> display(GxEPD2_300c(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEY0420F51 - 4C, 400x300px, 4.2"
#elif DISPLAY_ID == DT_GDEY0420F51
GxEPD2_4C<GxEPD2_420c_GDEY0420F51, GxEPD2_420c_GDEY0420F51::HEIGHT> display(GxEPD2_420c_GDEY0420F51(PIN_SS, PIN_DC,
                                                                                                    PIN_RST, PIN_BUSY));

// WS437YRBW - 4C, 512x368px, 4.37"
#elif DISPLAY_ID == DT_WS437YRBW
GxEPD2_4C<GxEPD2_437c, CALC_PAGE_HEIGHT(GxEPD2_437c::HEIGHT, GxEPD2_437c::WIDTH, 4)> display(GxEPD2_437c(PIN_SS, PIN_DC,
                                                                                                         PIN_RST,
                                                                                                         PIN_BUSY));

// GDEY0579F51 - 4C, 792x272px, 5.79"
#elif DISPLAY_ID == DT_GDEY0579F51
GxEPD2_4C<GxEPD2_0579c_GDEY0579F51, GxEPD2_0579c_GDEY0579F51::HEIGHT> display(GxEPD2_0579c_GDEY0579F51(PIN_SS, PIN_DC,
                                                                                                       PIN_RST,
                                                                                                       PIN_BUSY));

// GDEM075F52 - 4C, 800x480px, 7.5"
#elif DISPLAY_ID == DT_GDEM075F52
GxEPD2_4C<GxEPD2_750c_GDEM075F52, GxEPD2_750c_GDEM075F52::HEIGHT> display(GxEPD2_750c_GDEM075F52(PIN_SS, PIN_DC,
                                                                                                 PIN_RST, PIN_BUSY));

// GDEY116F51 - 4C, 960x640px, 11.6"
#elif DISPLAY_ID == DT_GDEY116F51
GxEPD2_4C<GxEPD2_1160c_GDEY116F51, CALC_PAGE_HEIGHT(GxEPD2_1160c_GDEY116F51::HEIGHT, GxEPD2_1160c_GDEY116F51::WIDTH, 4)>
  display(GxEPD2_1160c_GDEY116F51(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

///////////////////////
// 7C
///////////////////////

// GDEP0565D90 - 7C, 600x448px, 5.65"
#elif DISPLAY_ID == DT_GDEP0565D90
GxEPD2_7C<GxEPD2_565c, CALC_PAGE_HEIGHT(GxEPD2_565c::HEIGHT, GxEPD2_565c::WIDTH, 4)> display(GxEPD2_565c(PIN_SS, PIN_DC,
                                                                                                         PIN_RST,
                                                                                                         PIN_BUSY));

// GDEY073D46 - 7C, 800x480px, 7.3"
#elif DISPLAY_ID == DT_GDEY073D46
GxEPD2_7C<GxEPD2_730c_GDEY073D46, CALC_PAGE_HEIGHT(GxEPD2_730c_GDEY073D46::HEIGHT, GxEPD2_730c_GDEY073D46::WIDTH, 4)>
  display(GxEPD2_730c_GDEY073D46(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

// GDEP073E01 - 7C, 800x480px, 7.3"
#elif DISPLAY_ID == DT_GDEP073E01
GxEPD2_7C<GxEPD2_730c_GDEP073E01, CALC_PAGE_HEIGHT(GxEPD2_730c_GDEP073E01::HEIGHT, GxEPD2_730c_GDEP073E01::WIDTH, 4)>
  display(GxEPD2_730c_GDEP073E01(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));

#endif

// Font
#include <gfxfont.h>
// #include "fonts/OpenSansSB_12px.h"
#include "fonts/OpenSansSB_14px.h"
#include "fonts/OpenSansSB_16px.h"
#include "fonts/OpenSansSB_18px.h"
#include "fonts/OpenSansSB_20px.h"
#include "fonts/OpenSansSB_24px.h"

#include <QRCodeGenerator.h>

// Get display width from selected display class
#ifdef CROWPANEL_ESP32S3_213
  // Override logical resolution to match rotated orientation for 2.13"
  #define DISPLAY_RESOLUTION_X 250
  #define DISPLAY_RESOLUTION_Y 122
#else
  #define DISPLAY_RESOLUTION_X display.epd2.WIDTH
  #define DISPLAY_RESOLUTION_Y display.epd2.HEIGHT
#endif

namespace Display
{

void init()
{
#ifdef REMAP_SPI
  // only CLK and MOSI are important for EPD
  hspi.begin(PIN_SPI_CLK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_SS); // swap pins
  display.epd2.selectSPI(hspi, SPISettings(4000000, MSBFIRST, SPI_MODE0));
#endif

#if (defined ES3ink) || (defined ESP32S3Adapter) || (defined ESPink_V3) || (defined ESPink_V35) ||                     \
  (defined CROWPANEL_ESP32S3_579) || (defined CROWPANEL_ESP32S3_42) || (defined CROWPANEL_ESP32S3_213) ||              \
  (defined SVERIO_PAPERBOARD_SPI)
  display.init(115200, true, 2, false); // S3 boards with special reset circuits
#else
  display.init();
#endif

// Default rotation for all displays; adjust per-board below
#if (defined CROWPANEL_ESP32S3_213) || (DISPLAY_ID == DT_WS27RBW264176)
  display.setRotation(3); // rotate 90 degrees
#else
  display.setRotation(0);
#endif

  display.fillScreen(GxEPD_WHITE);   // white background
  display.setTextColor(GxEPD_BLACK); // black font
}

void clear()
{
  Logger::log<Logger::Topic::DISP>("Clearing display...\n");

  init();

  // Enable power supply for ePaper
  Board::setEPaperPowerOn(true);
  delay(500);

  setToFullWindow();
  setToFirstPage();
  do
  {
    display.fillRect(0, 0, DISPLAY_RESOLUTION_X, DISPLAY_RESOLUTION_Y, GxEPD_WHITE);
  } while (setToNextPage());

  delay(100);
  // Disable power supply for ePaper
  Board::setEPaperPowerOn(false);

  Logger::log<Logger::Topic::DISP>("Display cleared.\n");
}

void setRotation(uint8_t rotation) { display.setRotation(rotation); }

uint16_t getWidth() { return display.width(); }

uint16_t getHeight() { return display.height(); }

uint16_t getResolutionX() { return DISPLAY_RESOLUTION_X; }

uint16_t getResolutionY() { return DISPLAY_RESOLUTION_Y; }

uint16_t getNumberOfPages() { return display.pages(); }

void initM5()
{
#ifdef M5StackCoreInk
  display.init(115200, false);
#endif
}

void powerOffM5()
{
#ifdef M5StackCoreInk
  display.powerOff();
#endif
}

void pixelInit()
{
#ifdef ES3ink
  pixel.begin();
  pixel.setBrightness(15);
  resetPixelColor(0, 150, 0, 0);
#endif
}

void resetPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b)
{
#ifdef ES3ink
  pixel.clear();
  pixel.setPixelColor(n, pixel.Color(r, g, b));
  pixel.show();
#endif
}

void drawPixel(int16_t xCord, int16_t yCord, uint16_t color) { display.drawPixel(xCord, yCord, color); }

void drawQrCode(const char *qrStr, int qrSize, int yCord, int xCord, byte qrSizeMulti)
{
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(qrSize)];
  qrcode_initText(&qrcode, qrcodeData, qrSize, ECC_LOW, qrStr);

  int qrSizeTemp = (4 * qrSize) + 17;
  // QR Code Starting Point
  int offset_x = xCord - (qrSizeTemp * 2);
  int offset_y = yCord - (qrSizeTemp * 2);

  for (int y = 0; y < qrcode.size; y++)
  {
    for (int x = 0; x < qrcode.size; x++)
    {
      int newX = offset_x + (x * qrSizeMulti);
      int newY = offset_y + (y * qrSizeMulti);

      display.fillRect(newX, newY, qrSizeMulti, qrSizeMulti,
                       qrcode_getModule(&qrcode, x, y) ? GxEPD_BLACK : GxEPD_WHITE);
    }
  }
}

void setTextPos(const String &text, int xCord, int yCord)
{
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor(xCord, (yCord + (h / 2)));
  display.print(text);
}

void centeredText(const String &text, int xCord, int yCord)
{
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor(xCord - (w / 2), (yCord + (h / 2)));
  display.println(text);
}

void setToFullWindow() { display.setFullWindow(); }

void setToPartialWindow(int16_t xCord, int16_t yCord, int16_t width, int16_t height)
{
  display.setPartialWindow(xCord, yCord, width, height);
}

bool supportsPartialRefresh() { return display.epd2.hasPartialUpdate; }

void setToFirstPage() { display.firstPage(); }

bool setToNextPage() { return display.nextPage(); }

// Busy callback for light sleep during display refresh
void busyCallbackLightSleep(const void *)
{
#ifndef M5StackCoreInk
  // Enter light sleep for short periods while display is refreshing
  // Wake up after 100ms to check BUSY status again
  esp_sleep_enable_timer_wakeup(100 * 1000);
  esp_light_sleep_start();
#endif
}

void enableLightSleepDuringRefresh(bool enable)
{
#ifndef M5StackCoreInk
  if (enable)
  {
    Logger::log<Logger::Topic::DISP>("Enabling light sleep during display refresh\n");
    display.epd2.setBusyCallback(busyCallbackLightSleep, nullptr);
  }
  else
  {
    display.epd2.setBusyCallback(nullptr, nullptr);
  }
#endif
}

void showNoWiFiError(uint64_t sleepMinutes, const String &wikiUrl)
{
  init();

  // Enable power supply for ePaper
  Board::setEPaperPowerOn(true);
  delay(500);

  setToFullWindow();
  setToFirstPage();
  do
  {
    display.fillRect(0, 0, DISPLAY_RESOLUTION_X, DISPLAY_RESOLUTION_Y, GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&OpenSansSB_20px);
    centeredText("Cannot connect to Wi-Fi", DISPLAY_RESOLUTION_X / 2, DISPLAY_RESOLUTION_Y / 2 - 15);
    display.setFont(&OpenSansSB_16px);
    centeredText("Retries in " + String(sleepMinutes) + " minutes.", DISPLAY_RESOLUTION_X / 2,
                 DISPLAY_RESOLUTION_Y / 2 + 15);
    display.setFont(&OpenSansSB_14px);
    centeredText("Docs: " + wikiUrl, DISPLAY_RESOLUTION_X / 2, DISPLAY_RESOLUTION_Y - 20);
  } while (setToNextPage());

  delay(100);
  // Disable power supply for ePaper
  Board::setEPaperPowerOn(false);
}

void showWiFiError(const String &hostname, const String &password, const String &urlWeb, const String &wikiUrl)
{
  /*
    QR code hint
    Common format: WIFI:S:<SSID>;T:<WEP|WPA|nopass>;P:<PASSWORD>;H:<true|false|blank>;;
    Sample: WIFI:S:MySSID;T:WPA;P:MyPassW0rd;;
  */
  const String qrString = "WIFI:S:" + hostname + ";T:WPA;P:" + password + ";;";
  // Logger::log<Logger::Topic::WIFI>("Generated string: {}\n", qrString);

  init();

  // Enable power supply for ePaper
  Board::setEPaperPowerOn(true);
  delay(500);

  setToFullWindow();
  setToFirstPage();
  do
  {
    if (DISPLAY_RESOLUTION_X >= 800)
    {
      display.fillRect(0, 0, DISPLAY_RESOLUTION_X, 90, GxEPD_BLACK);
      display.setTextColor(GxEPD_WHITE);
      display.setFont(&OpenSansSB_24px);
      centeredText("No Wi-Fi configured OR connection lost", DISPLAY_RESOLUTION_X / 2, 28);
      display.setFont(&OpenSansSB_18px);
      centeredText("Retries in a few minutes if lost.", DISPLAY_RESOLUTION_X / 2, 64);
      display.setTextColor(GxEPD_BLACK);
      centeredText("To setup or change Wi-Fi configuration", DISPLAY_RESOLUTION_X / 2, 120);
      centeredText("(with mobile data turned off):", DISPLAY_RESOLUTION_X / 2, 145);
      centeredText("1) Connect to this AP:", DISPLAY_RESOLUTION_X / 4, (DISPLAY_RESOLUTION_Y / 2) - 50);
      centeredText("2) Open in web browser:", DISPLAY_RESOLUTION_X * 3 / 4, (DISPLAY_RESOLUTION_Y / 2) - 50);

      drawQrCode(qrString.c_str(), 4, (DISPLAY_RESOLUTION_Y / 2) + 40, DISPLAY_RESOLUTION_X / 4, 4);
      display.drawLine(DISPLAY_RESOLUTION_X / 2 - 1, (DISPLAY_RESOLUTION_Y / 2) - 60, DISPLAY_RESOLUTION_X / 2 - 1,
                       (DISPLAY_RESOLUTION_Y / 2) + 170, GxEPD_BLACK);
      display.drawLine(DISPLAY_RESOLUTION_X / 2, (DISPLAY_RESOLUTION_Y / 2) - 60, DISPLAY_RESOLUTION_X / 2,
                       (DISPLAY_RESOLUTION_Y / 2) + 170, GxEPD_BLACK);
      drawQrCode(urlWeb.c_str(), 4, (DISPLAY_RESOLUTION_Y / 2) + 40, DISPLAY_RESOLUTION_X * 3 / 4, 4);

      centeredText("SSID: " + hostname, DISPLAY_RESOLUTION_X / 4, (DISPLAY_RESOLUTION_Y / 2) + 130);
      centeredText("Password: " + password, DISPLAY_RESOLUTION_X / 4, (DISPLAY_RESOLUTION_Y / 2) + 155);
      centeredText(urlWeb, DISPLAY_RESOLUTION_X * 3 / 4, (DISPLAY_RESOLUTION_Y / 2) + 130);
      display.fillRect(0, DISPLAY_RESOLUTION_Y - 40, DISPLAY_RESOLUTION_X, DISPLAY_RESOLUTION_Y, GxEPD_BLACK);
      display.setTextColor(GxEPD_WHITE);
      centeredText("Documentation: " + wikiUrl, DISPLAY_RESOLUTION_X / 2, DISPLAY_RESOLUTION_Y - 22);
    }
    else if (DISPLAY_RESOLUTION_X >= 600)
    {
      display.fillRect(0, 0, DISPLAY_RESOLUTION_X, 70, GxEPD_BLACK);
      display.setTextColor(GxEPD_WHITE);
      display.setFont(&OpenSansSB_20px);
      centeredText("No Wi-Fi configured OR connection lost", DISPLAY_RESOLUTION_X / 2, 20);
      display.setFont(&OpenSansSB_14px);
      centeredText("Retries in a few minutes if lost.", DISPLAY_RESOLUTION_X / 2, 50);
      display.setTextColor(GxEPD_BLACK);
      centeredText("To setup or change Wi-Fi configuration", DISPLAY_RESOLUTION_X / 2, 90);
      centeredText("(with mobile data turned off):", DISPLAY_RESOLUTION_X / 2, 110);
      centeredText("1) Connect to this AP:", DISPLAY_RESOLUTION_X / 4, 140);
      centeredText("2) Open in web browser:", DISPLAY_RESOLUTION_X * 3 / 4, 140);
      int qrScale = (DISPLAY_RESOLUTION_Y < 280) ? 2 : 3;
      drawQrCode(qrString.c_str(), 4, 225, DISPLAY_RESOLUTION_X / 4 + 18, qrScale);
      display.drawLine(DISPLAY_RESOLUTION_X / 2 - 1, 135, DISPLAY_RESOLUTION_X / 2 - 1, 310, GxEPD_BLACK);
      display.drawLine(DISPLAY_RESOLUTION_X / 2, 135, DISPLAY_RESOLUTION_X / 2, 310, GxEPD_BLACK);
      drawQrCode(urlWeb.c_str(), 4, 225, DISPLAY_RESOLUTION_X * 3 / 4 + 18, qrScale);
      centeredText("SSID: " + hostname, DISPLAY_RESOLUTION_X / 4, 280);
      centeredText("Password: " + password, DISPLAY_RESOLUTION_X / 4, 300);
      centeredText(urlWeb, DISPLAY_RESOLUTION_X * 3 / 4, 280);
      display.fillRect(0, DISPLAY_RESOLUTION_Y - 36, DISPLAY_RESOLUTION_X, DISPLAY_RESOLUTION_Y, GxEPD_BLACK);
      display.setTextColor(GxEPD_WHITE);
      centeredText("Documentation: " + wikiUrl, DISPLAY_RESOLUTION_X / 2, DISPLAY_RESOLUTION_Y - 24);
    }
    else if (DISPLAY_RESOLUTION_X >= 400)
    {
      display.fillRect(0, 0, DISPLAY_RESOLUTION_X, 58, GxEPD_BLACK);
      display.setTextColor(GxEPD_WHITE);
      display.setFont(&OpenSansSB_16px);
      centeredText("No Wi-Fi configured OR connection lost", DISPLAY_RESOLUTION_X / 2, 16);
      display.setFont(&OpenSansSB_14px);
      centeredText("Retries in a few minutes if lost.", DISPLAY_RESOLUTION_X / 2, 40);
      display.setTextColor(GxEPD_BLACK);
      centeredText("To setup or change Wi-Fi configuration", DISPLAY_RESOLUTION_X / 2, 72);
      centeredText("(with mobile data turned off):", DISPLAY_RESOLUTION_X / 2, 92);
      centeredText("1) Connect to AP", DISPLAY_RESOLUTION_X / 4, 115);
      centeredText("2) Open in browser:", DISPLAY_RESOLUTION_X * 3 / 4, 115);
      int qrScale = (DISPLAY_RESOLUTION_Y < 280) ? 2 : 3;
      drawQrCode(qrString.c_str(), 3, 190, DISPLAY_RESOLUTION_X / 4 + 18, qrScale);
      display.drawLine(DISPLAY_RESOLUTION_X / 2 + 2, 108, DISPLAY_RESOLUTION_X / 2 + 2, 260, GxEPD_BLACK);
      display.drawLine(DISPLAY_RESOLUTION_X / 2 + 3, 108, DISPLAY_RESOLUTION_X / 2 + 3, 260, GxEPD_BLACK);
      drawQrCode(urlWeb.c_str(), 3, 190, DISPLAY_RESOLUTION_X * 3 / 4 + 18, qrScale);
      centeredText("AP: " + hostname, DISPLAY_RESOLUTION_X / 4, 232);
      centeredText("Password: " + password, DISPLAY_RESOLUTION_X / 4, 250);
      centeredText(urlWeb, DISPLAY_RESOLUTION_X * 3 / 4, 232);
      display.fillRect(0, DISPLAY_RESOLUTION_Y - 25, DISPLAY_RESOLUTION_X, DISPLAY_RESOLUTION_Y, GxEPD_BLACK);
      display.setTextColor(GxEPD_WHITE);
      centeredText("Documentation: " + wikiUrl, DISPLAY_RESOLUTION_X / 2, DISPLAY_RESOLUTION_Y - 15);
    }
    else
    {
      // Initialize defined resolution into variables for possible swap later
      uint16_t small_resolution_x = DISPLAY_RESOLUTION_X;
      uint16_t small_resolution_y = DISPLAY_RESOLUTION_Y;

      // Use landscape mode - many small displays are in portrait mode
      if (DISPLAY_RESOLUTION_X < DISPLAY_RESOLUTION_Y)
      {
        setRotation(3);

        // Swap resolution for X and Y
        small_resolution_x = DISPLAY_RESOLUTION_Y;
        small_resolution_y = DISPLAY_RESOLUTION_X;
      }

      display.fillRect(0, 0, small_resolution_x, 34, GxEPD_BLACK);
      display.setTextColor(GxEPD_WHITE);
      display.setFont(&OpenSansSB_14px);
      centeredText("No Wi-Fi setup OR connection", small_resolution_x / 2, 6);
      centeredText("Retries in a few minutes if lost.", small_resolution_x / 2, 25);
      display.setTextColor(GxEPD_BLACK);
      setTextPos("Setup or change cfg:", 2, 44);
      setTextPos("AP: ..." + hostname.substring(hostname.length() - 6), 2, 64);
      setTextPos("Password: " + password, 2, 84);
      setTextPos("Help: zivyobraz.eu ", 2, 104);

      drawQrCode(qrString.c_str(), 3, 93, small_resolution_x - 28, 3);
    }
  } while (setToNextPage());

  delay(100);
  // Disable power supply for ePaper
  Board::setEPaperPowerOn(false);
}
} // namespace Display
