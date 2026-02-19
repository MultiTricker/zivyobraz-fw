#ifdef USE_EPDIY_DRIVER

  #include "board.h"
  #include "display.h"
  #include "epdiy_gxepd2_bridge.h"

  // Map DISPLAY_TYPE to epdiy display descriptor
  #if DISPLAY_ID == DT_ED097TC2_EPDIY
    #define EPDIY_DISPLAY ED097TC2
    #ifndef EPDIY_VCOM
      #define EPDIY_VCOM 1500
    #endif
  #elif DISPLAY_ID == DT_ED060XC3_EPDIY
    #define EPDIY_DISPLAY ED060XC3
    #ifndef EPDIY_VCOM
      #define EPDIY_VCOM 300
    #endif
  #else
    #error "Unknown epdiy display type"
  #endif

  // Map BOARD_TYPE to epdiy board descriptor
  #if defined(SVERIO_PAPERBOARD_EPDIY)
    #define EPDIY_BOARD sverio_paperboard_v1
  #else
    #error "No epdiy board mapping for this BOARD_TYPE"
  #endif

static uint8_t colorToEpdiy(uint16_t color)
{
  switch (color)
  {
    case GxEPD_WHITE:
      return 0xFF;
    case GxEPD_BLACK:
      return 0x00;
    case GxEPD_LIGHTGREY:
      return 0xDD;
    case GxEPD_DARKGREY:
      return 0x88;
    default:
      return 0xFF;
  }
}

EpdiyDisplay::EpdiyEpd2::EpdiyEpd2(EpdiyDisplay *owner) : WIDTH(0), HEIGHT(0), hasPartialUpdate(false), m_owner(owner)
{
}

void EpdiyDisplay::EpdiyEpd2::selectSPI(SPIClass &spi, SPISettings settings)
{
  (void)spi;
  (void)settings;
}

void EpdiyDisplay::EpdiyEpd2::setPaged() {}

void EpdiyDisplay::EpdiyEpd2::setBusyCallback(void (*callback)(const void *)) { (void)callback; }

void EpdiyDisplay::EpdiyEpd2::setBusyCallback(void (*callback)(const void *), void *context)
{
  (void)callback;
  (void)context;
}

void EpdiyDisplay::EpdiyEpd2::refresh(bool partial) { m_owner->refreshDisplay(partial); }

void EpdiyDisplay::EpdiyEpd2::writeImage(const uint8_t *black, const uint8_t *color, int16_t x, int16_t y, int16_t w,
                                         int16_t h, bool invert, bool mirror, bool pgm)
{
  (void)color;
  (void)invert;
  (void)mirror;
  (void)pgm;

  m_owner->ensureInit();
  if (!black)
    return;
  if (!m_owner->m_framebuffer)
    return;

  const int fb_width = epd_width();
  uint8_t *fb = m_owner->m_framebuffer;

  for (int16_t row = 0; row < h; row++)
  {
    const int16_t py = y + row;
    for (int16_t col = 0; col < w; col++)
    {
      uint16_t idx = (row * ((w + 7) / 8)) + (col / 8);
      uint8_t bit = 7 - (col % 8);
      bool is_black = (black[idx] >> bit) & 0x01;
      uint8_t gray = is_black ? 0x00 : 0x0F;

      // epdiy framebuffer: even x = low nibble, odd x = high nibble
      const int16_t px = x + col;
      int fb_idx = py * (fb_width / 2) + px / 2;
      if (px & 1)
        fb[fb_idx] = (fb[fb_idx] & 0x0F) | (gray << 4);
      else
        fb[fb_idx] = (fb[fb_idx] & 0xF0) | gray;
    }
  }
}

void EpdiyDisplay::EpdiyEpd2::writeImage(const uint8_t *black, int16_t x, int16_t y, int16_t w, int16_t h, bool invert,
                                         bool mirror, bool pgm)
{
  writeImage(black, nullptr, x, y, w, h, invert, mirror, pgm);
}

void EpdiyDisplay::EpdiyEpd2::writeImage_4G(const uint8_t *data, uint8_t level, int16_t x, int16_t y, int16_t w,
                                            int16_t h, bool invert, bool mirror, bool pgm)
{
  (void)level;
  (void)invert;
  (void)mirror;
  (void)pgm;

  m_owner->ensureInit();
  if (!data)
    return;
  if (!m_owner->m_framebuffer)
    return;

  static const uint8_t lut_2bit_to_4bit[] = {0x00, 0x08, 0x0D, 0x0F};
  const int fb_width = epd_width();
  uint8_t *fb = m_owner->m_framebuffer;

  for (int16_t row = 0; row < h; row++)
  {
    const int16_t py = y + row;
    for (int16_t col = 0; col < w; col++)
    {
      uint16_t idx = (row * ((w + 3) / 4)) + (col / 4);
      uint8_t bit_pos = (3 - (col % 4)) * 2;
      uint8_t pixel_val = (data[idx] >> bit_pos) & 0x03;
      uint8_t gray = lut_2bit_to_4bit[pixel_val];

      // epdiy framebuffer: even x = low nibble, odd x = high nibble
      const int16_t px = x + col;
      int fb_idx = py * (fb_width / 2) + px / 2;
      if (px & 1)
        fb[fb_idx] = (fb[fb_idx] & 0x0F) | (gray << 4);
      else
        fb[fb_idx] = (fb[fb_idx] & 0xF0) | gray;
    }
  }
}

void EpdiyDisplay::EpdiyEpd2::writeNative(const uint8_t *data, const uint8_t *color, int16_t x, int16_t y, int16_t w,
                                          int16_t h, bool invert, bool mirror, bool pgm)
{
  (void)data;
  (void)color;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)invert;
  (void)mirror;
  (void)pgm;
}

EpdiyDisplay::EpdiyDisplay()
    : Adafruit_GFX(0, 0), epd2(this), m_framebuffer(nullptr), m_initialized(false), m_pageActive(false)
{
}

void EpdiyDisplay::init()
{
  if (m_initialized)
    return;

  epd_init(&EPDIY_BOARD, &EPDIY_DISPLAY, EPD_LUT_64K);
  epd_set_vcom(EPDIY_VCOM);
  m_hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
  epd_set_rotation(EPD_ROT_LANDSCAPE);
  m_framebuffer = epd_hl_get_framebuffer(&m_hl);
  m_initialized = true;
  updateDimensions();
}

void EpdiyDisplay::init(uint32_t baud)
{
  (void)baud;
  init();
}

void EpdiyDisplay::init(uint32_t baud, bool initial, uint16_t reset, bool pulldown)
{
  (void)baud;
  (void)initial;
  (void)reset;
  (void)pulldown;
  init();
}

void EpdiyDisplay::powerOff() { epd_poweroff(); }

void EpdiyDisplay::setRotation(uint8_t rotation)
{
  EpdRotation rot = EPD_ROT_LANDSCAPE;
  switch (rotation)
  {
    case 0:
      rot = EPD_ROT_LANDSCAPE;
      break;
    case 1:
      rot = EPD_ROT_PORTRAIT;
      break;
    case 2:
      rot = EPD_ROT_INVERTED_LANDSCAPE;
      break;
    case 3:
      rot = EPD_ROT_INVERTED_PORTRAIT;
      break;
  }
  epd_set_rotation(rot);
  updateDimensions();
}

void EpdiyDisplay::setFullWindow() {}

void EpdiyDisplay::setPartialWindow(int16_t x, int16_t y, int16_t w, int16_t h)
{
  (void)x;
  (void)y;
  (void)w;
  (void)h;
}

void EpdiyDisplay::fillScreen(uint16_t color) { fillRect(0, 0, width(), height(), color); }

void EpdiyDisplay::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  ensureInit();
  if (!m_framebuffer)
    return;

  EpdRect rect = {x, y, w, h};
  epd_fill_rect(rect, colorToEpdiy(color), m_framebuffer);
}

void EpdiyDisplay::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  ensureInit();
  if (!m_framebuffer)
    return;

  epd_draw_pixel(x, y, colorToEpdiy(color), m_framebuffer);
}

void EpdiyDisplay::drawPixel8bit(int16_t x, int16_t y, uint8_t gray)
{
  ensureInit();
  if (!m_framebuffer)
    return;

  epd_draw_pixel(x, y, gray, m_framebuffer);
}

void EpdiyDisplay::firstPage()
{
  ensureInit();
  m_pageActive = true;

  if (m_framebuffer)
  {
    memset(m_framebuffer, 0xFF, epd_width() * epd_height() / 2);
  }
}

bool EpdiyDisplay::nextPage()
{
  if (!m_pageActive)
    return false;
  refreshDisplay(false);
  m_pageActive = false;
  return false;
}

uint16_t EpdiyDisplay::pages() const { return 1; }

void EpdiyDisplay::ensureInit()
{
  if (!m_initialized)
    init();
}

void EpdiyDisplay::updateDimensions()
{
  epd2.WIDTH = epd_width();
  epd2.HEIGHT = epd_height();
  _width = epd_rotated_display_width();
  _height = epd_rotated_display_height();
}

void EpdiyDisplay::refreshDisplay(bool partial)
{
  (void)partial;
  ensureInit();

  epd_poweron();
  epd_clear();
  epd_hl_update_screen(&m_hl, MODE_EPDIY_WHITE_TO_GL16, epd_ambient_temperature());
  epd_poweroff();
}

#endif // USE_EPDIY_DRIVER
