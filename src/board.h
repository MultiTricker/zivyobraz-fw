#ifndef BOARD_H
#define BOARD_H

/////////////////////////////////
// Uncomment for correct board
/////////////////////////////////

// #define ESPink_V2 // LáskaKit ESPInk 2.x, ESP32-WROOM-32, ADC battery measurement
// #define ESPink_V3 // LáskaKit ESPInk 3.0-3.4, ESP32-S3, with FuelGauge, about 100 pcs
// #define ESPink_V35 // LáskaKit ESPInk 3.5, ESP32-S3, ADC battery measurement, extra buton
// #define ESP32S3Adapter // LáskaKit ESP32-S3 with adapter for 6/7 color ePaper displays
// #define ES3ink // Board from dronecz
// #define SEEEDSTUDIO_XIAO_ESP32C3 // Seeed Studio XIAO ESP32C3, bundled with 800x480 BW display
// #define SEEEDSTUDIO_XIAO_EDDB_ESP32S3 //Development board distributed as part of the TRMNL 7.5" (OG) DIY Kit
// #define MakerBadge_revB // also works with A and C
// #define MakerBadge_revD
// #define REMAP_SPI
// #define TTGO_T5_v23 // tested only with 2.13" variant
// #define CROWPANEL_ESP32S3_579 // Elecrow CrowPanel 5.79" 272x792, ESP32-S3-WROOM-1
// #define CROWPANEL_ESP32S3_42  // Elecrow CrowPanel 4.2" 400x300, ESP32-S3-WROOM-1
// #define CROWPANEL_ESP32S3_213 // Elecrow CrowPanel 2.13" 250x122, ESP32-S3-WROOM-1
// #define WS_EPAPER_ESP32_BOARD // Waveshare ESP32 Driver Board
// #define SVERIO_PAPERBOARD_SPI // Custom ESP32-S3 board with SPI ePaper (SVERIO)

#include <Arduino.h>

#ifdef ESPink_V2
  #define PIN_SS 5
  #define PIN_DC 17
  #define PIN_RST 16
  #define PIN_BUSY 4
  #define PIN_CS2 35
  #define ePaperPowerPin 2
  // Maximum page buffer size in bytes (ESP32-WROOM has limited RAM)
  #define BOARD_MAX_PAGE_BUFFER_SIZE (48 * 1024)

#elif defined ESPink_V3
  #define PIN_SS 10
  #define PIN_DC 48
  #define PIN_RST 45
  #define PIN_BUSY 36
  #define PIN_CS2 35
  #define ePaperPowerPin 47
  #define PIN_SPI_MOSI 11
  #define PIN_SPI_CLK 12
  #define PIN_SDA 42
  #define PIN_SCL 2
  #define PIN_ALERT 9
  // ESP32-S3 with PSRAM - large buffer for single-page rendering
  #define BOARD_MAX_PAGE_BUFFER_SIZE (200 * 1024)

#elif defined ESPink_V35
  #define PIN_SS 10
  #define PIN_DC 48
  #define PIN_RST 45
  #define PIN_BUSY 38
  #define PIN_CS2 35
  #define ePaperPowerPin 47
  #define PIN_SPI_MOSI 11
  #define PIN_SPI_CLK 12
  #define PIN_SDA 42
  #define PIN_SCL 2
  #define vBatPin 9
  #define dividerRatio (1.7693877551f)
  // ESP32-S3 with PSRAM - large buffer for single-page rendering
  #define BOARD_MAX_PAGE_BUFFER_SIZE (200 * 1024)

#elif defined ESP32S3Adapter
  // With ESP32-S3 DEVKIT from laskakit.cz
  #define PIN_SS 10
  #define PIN_DC 41
  #define PIN_RST 40
  #define PIN_BUSY 13
  #define ePaperPowerPin 47
  #define PIN_SPI_CLK 12
  #define PIN_SPI_MOSI 11
  #define PIN_SDA 42
  #define PIN_SCL 2
  #define vBatPin 9
  #define dividerRatio (1.7693877551f)
  // ESP32-S3 with PSRAM - large buffer for single-page rendering
  #define BOARD_MAX_PAGE_BUFFER_SIZE (200 * 1024)

#elif defined ES3ink
  // for version P1.1
  #define PIN_SS 10
  #define PIN_DC 7
  #define PIN_RST 5
  #define PIN_BUSY 6
  #define PIN_CS2 35
  #define ePaperPowerPin 3
  #define enableBattery 40
  #define RGBledPin 48
  #define RGBledPowerPin 14
  #define vBatPin ADC1_GPIO2_CHANNEL
  #define dividerRatio (2.018f)
  // ESP32-S3 with PSRAM - large buffer for single-page rendering
  #define BOARD_MAX_PAGE_BUFFER_SIZE (200 * 1024)

#elif defined MakerBadge_revB
  #define PIN_SS 41
  #define PIN_DC 40
  #define PIN_RST 39
  #define PIN_BUSY 42
  #define ePaperPowerPin 16
  #define vBatPin 6
  #define BATT_V_CAL_SCALE (1.00f)
  // ESP32-S3 with PSRAM - large buffer for single-page rendering
  #define BOARD_MAX_PAGE_BUFFER_SIZE (200 * 1024)

#elif defined MakerBadge_revD
  #define PIN_SS 41
  #define PIN_DC 40
  #define PIN_RST 39
  #define PIN_BUSY 42
  #define ePaperPowerPin 16
  #define enableBattery 14
  #define vBatPin 6
  #define BATT_V_CAL_SCALE (1.05f)
  // ESP32-S3 with PSRAM - large buffer for single-page rendering
  #define BOARD_MAX_PAGE_BUFFER_SIZE (200 * 1024)

#elif defined TTGO_T5_v23
  #define PIN_SS 5
  #define PIN_DC 17
  #define PIN_RST 16
  #define PIN_BUSY 4
  #define ePaperPowerPin 2
  #define vBatPin 35
  // Maximum page buffer size in bytes (ESP32 has limited RAM)
  #define BOARD_MAX_PAGE_BUFFER_SIZE (48 * 1024)

#elif defined SEEEDSTUDIO_XIAO_ESP32C3
  #define PIN_SS 3
  #define PIN_DC 5
  #define PIN_RST 2
  #define PIN_BUSY 4
  #define ePaperPowerPin 7
  #define PIN_SPI_CLK 8
  #define PIN_SPI_MOSI 11
  // Maximum page buffer size in bytes (ESP32-C3 has limited RAM)
  #define BOARD_MAX_PAGE_BUFFER_SIZE (48 * 1024)

#elif defined SEEEDSTUDIO_XIAO_EDDB_ESP32S3
  #define PIN_SS 44
  #define PIN_DC 10
  #define PIN_RST 38
  #define PIN_BUSY 4
  #define ePaperPowerPin 43
  #define PIN_SPI_CLK 7
  #define PIN_SPI_MOSI 9
  #define enableBattery 6
  #define vBatPin 1
  #define dividerRatio (2.000f)
  // ESP32-S3 with PSRAM - large buffer for single-page rendering
  #define BOARD_MAX_PAGE_BUFFER_SIZE (200 * 1024)

#elif (defined CROWPANEL_ESP32S3_579) || (defined CROWPANEL_ESP32S3_42)
  #define PIN_SS 45
  #define PIN_DC 46
  #define PIN_RST 47
  #define PIN_BUSY 48
  #define ePaperPowerPin 7
  #define PIN_SPI_CLK 12
  #define PIN_SPI_MISO -1
  #define PIN_SPI_MOSI 11
  #define PIN_SPI_SS PIN_SS
  #define vBatPin -1
  // ESP32-S3 with PSRAM - large buffer for single-page rendering
  #define BOARD_MAX_PAGE_BUFFER_SIZE (200 * 1024)

#elif defined CROWPANEL_ESP32S3_213
  #define PIN_SS 14
  #define PIN_DC 13
  #define PIN_RST 10
  #define PIN_BUSY 9
  #define ePaperPowerPin 7
  #define PIN_SPI_CLK 12
  #define PIN_SPI_MISO -1
  #define PIN_SPI_MOSI 11
  #define PIN_SPI_SS PIN_SS
  #define vBatPin -1
  // ESP32-S3 with PSRAM - large buffer for single-page rendering
  #define BOARD_MAX_PAGE_BUFFER_SIZE (200 * 1024)

#elif defined WS_EPAPER_ESP32_BOARD
  #define PIN_SS 15
  #define PIN_DC 27
  #define PIN_RST 26
  #define PIN_BUSY 25
  #define PIN_CS2 35
  #define ePaperPowerPin 2
  // Maximum page buffer size in bytes (ESP32 has limited RAM)
  #define BOARD_MAX_PAGE_BUFFER_SIZE (48 * 1024)

  #define REMAP_SPI
  #define PIN_SPI_CLK 13
  #define PIN_SPI_MOSI 14
  #define PIN_SPI_MISO -1
  #define PIN_SPI_SS -1

#elif defined SVERIO_PAPERBOARD_SPI
  #define PIN_SS 12
  #define PIN_DC 13
  #define PIN_RST 14
  #define PIN_BUSY 21
  #define PIN_SPI_MOSI 10
  #define PIN_SPI_CLK 11
  #define PIN_SPI_MISO -1
  #define PIN_SPI_SS PIN_SS
  #define PIN_SDA 39
  #define PIN_SCL 40
  #define ePaperPowerPin 41
  #define enableBattery 2
  #define vBatPin ADC1_GPIO1_CHANNEL
  #define dividerRatio (2.7507665f)
  // ESP32-S3 with PSRAM - large buffer for single-page rendering
  #define BOARD_MAX_PAGE_BUFFER_SIZE (200 * 1024)

#else
  #error "Board not defined!"
#endif

#ifdef REMAP_SPI
  #if !defined PIN_SPI_CLK
    #define PIN_SPI_CLK 13 // CLK
  #endif
  #if !defined PIN_SPI_MISO
    #define PIN_SPI_MISO 14 // unused
  #endif
  #if !defined PIN_SPI_MOSI
    #define PIN_SPI_MOSI 12 // DIN
  #endif
  #if !defined PIN_SPI_SS
    #define PIN_SPI_SS 15 // unused
  #endif
#endif

#if !defined vBatPin
  #ifdef M5StackCoreInk
    #define vBatPin 35
  #else
    #define vBatPin 34
  #endif
#endif
#ifndef dividerRatio
  #define dividerRatio (1.769f)
#endif

namespace Board
{
void setupHW();
void setEPaperPowerOn(bool on);
void enterDeepSleepMode(uint64_t sleepDuration);

float getBatteryVoltage();
} // namespace Board

#endif // BOARD_H
