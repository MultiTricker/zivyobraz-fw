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

#ifdef TYPE_8G
  // Extended grayscale levels for 8-level grayscale displays (epdiy 4bpp)
  // RGB565-encoded gray values used by mapColorValue for Z-format indices 2,4 and 6
  #define GxEPD_GRAY_EE 0xEF7D // 238
  #define GxEPD_GRAY_DD 0xDEFB // 221
  #define GxEPD_GRAY_CC 0xCE79 // 204
  #define GxEPD_GRAY_BB 0xBDD7 // 187
  #define GxEPD_GRAY_88 0x8C71 // 136
  #define GxEPD_GRAY_44 0x4228 // 68

  // Other grayscale levels for reference (not currently used)
  /*
  #define GxEPD_GRAY_AA   0xAD55 // 170
  #define GxEPD_GRAY_99   0x9CF3 // 153
  #define GxEPD_GRAY_77   0x7BEF // 119
  #define GxEPD_GRAY_66   0x6B4D // 102
  #define GxEPD_GRAY_55   0x52AA // 85
  #define GxEPD_GRAY_33   0x3186 // 51
  #define GxEPD_GRAY_22   0x2104 // 34
  #define GxEPD_GRAY_11   0x1082 // 17
  */
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
