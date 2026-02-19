#ifndef EPDIY_GXEPD2_BRIDGE_H
#define EPDIY_GXEPD2_BRIDGE_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <epdiy.h>

#ifndef GxEPD_WHITE
  #define GxEPD_WHITE 0xFFFF
  #define GxEPD_BLACK 0x0000
  #define GxEPD_LIGHTGREY 0xC618
  #define GxEPD_DARKGREY 0x7BEF
  #define GxEPD_RED 0xF800
  #define GxEPD_YELLOW 0xFFE0
  #define GxEPD_GREEN 0x07E0
  #define GxEPD_BLUE 0x001F
  #define GxEPD_ORANGE 0xFD20
#endif

class EpdiyDisplay : public Adafruit_GFX
{
public:
  struct EpdiyEpd2
  {
    explicit EpdiyEpd2(EpdiyDisplay *owner);

    int16_t WIDTH;
    int16_t HEIGHT;
    bool hasPartialUpdate;

    void selectSPI(SPIClass &spi, SPISettings settings);
    void setPaged();
    void setBusyCallback(void (*callback)(const void *));
    void setBusyCallback(void (*callback)(const void *), void *context);
    void refresh(bool partial);
    void writeImage(const uint8_t *black, const uint8_t *color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert,
                    bool mirror, bool pgm);
    void writeImage(const uint8_t *black, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror,
                    bool pgm);
    void writeImage_4G(const uint8_t *data, uint8_t level, int16_t x, int16_t y, int16_t w, int16_t h, bool invert,
                       bool mirror, bool pgm);
    void writeNative(const uint8_t *data, const uint8_t *color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert,
                     bool mirror, bool pgm);

  private:
    EpdiyDisplay *m_owner;
  };

  EpdiyDisplay();

  void init();
  void init(uint32_t baud);
  void init(uint32_t baud, bool initial, uint16_t reset, bool pulldown);
  void powerOff();

  void setRotation(uint8_t rotation);
  void setFullWindow();
  void setPartialWindow(int16_t x, int16_t y, int16_t w, int16_t h);

  void fillScreen(uint16_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;

  // Direct 8-bit grayscale pixel drawing (bypasses GxEPD2 4-level limitation)
  // gray: 0x00 (black) to 0xFF (white), supports 16 levels (0x00, 0x11, ... 0xFF)
  void drawPixel8bit(int16_t x, int16_t y, uint8_t gray);

  void firstPage();
  bool nextPage();
  uint16_t pages() const;

  EpdiyEpd2 epd2;

private:
  friend struct EpdiyEpd2;

  void ensureInit();
  void updateDimensions();
  void refreshDisplay(bool partial);

  EpdiyHighlevelState m_hl;
  uint8_t *m_framebuffer;
  bool m_initialized;
  bool m_pageActive;
};

#endif
