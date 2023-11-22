/*
 * ZivyObraz.eu - Orchestrate your ePaper displays
 * 
 * You need to change some initial things like ePaper type etc. - see below.
 *
 * Default password for Wi-Fi AP is: zivyobraz 
 *
 * Code based on both: 
 * https://github.com/ZinggJM/GxEPD2/blob/master/examples/GxEPD2_WiFi_Example/GxEPD2_WiFi_Example.ino
 * and
 * https://github.com/LaskaKit/ESPink-42/blob/main/SW/Weatherstation_info/Weatherstation_info.ino
 * 
 * Compile with board type: ESP32 dev module
 *
 * Libraries:
 * Analog read with calibration data: https://github.com/madhephaestus/ESP32AnalogRead/ 
 * EPD library: https://github.com/ZinggJM/GxEPD2
 * EPD library for 4G "Grayscale": https://github.com/ZinggJM/GxEPD2_4G
 * WiFi manager by tzapu https://github.com/tzapu/WiFiManager
 *
 * original code made by Jean-Marc Zingg and LaskaKit
 * modified by @MultiTricker (https://michalsevcik.eu)
 */

/////////////////////////////////
// Uncomment for correct board
/////////////////////////////////

//#define ESPink
//#define ES3ink
//#define REMAP_SPI
//#define MakerBadge_revB //also works with A and C
// #define MakerBadge_revD

//////////////////////////////////////////////////////////////////////////////////////
// Uncomment if you have connected SHT40 sensor for sending temperature and humidity
//////////////////////////////////////////////////////////////////////////////////////

//#define SHT40

//////////////////////////////////////////////////////////////
// Uncomment correct color capability of your ePaper display
//////////////////////////////////////////////////////////////

//#define TYPE_BW // black and white
//#define TYPE_3C // 3 colors - black, white and red/yellow
//#define TYPE_GRAYSCALE // grayscale - 4 colors
//#define TYPE_7C // 7 colors

//////////////////////////////////////////////////////////////
// Uncomment for correct ePaper you have
//////////////////////////////////////////////////////////////

// BW
//#define D_B74           // 122x250, 2.13" MakerBadge all versions
//#define D_GDEY0213B7    // 122x250, 2.13"
//#define D_GDEY027T91    // 176x246, 2.7"
//#define D_GDEW0154T8    // 152x152, 1.54"
//#define D_GDEW042T2     // 400x300, 4.2"
//#define D_GDEQ042T81    // 400x300, 4.2"
//#define D_GDEQ0583T31   // 648x480, 5.83"
//#define D_GDEW075T7     // 800x480, 7.5"
//#define D_GDEQ102T90    // 960x640, 10.2"

// Grayscale
//#define D_GDEW042T2_G   // 400x300, 4.2"
//#define D_GDEW075T7_G   // 800x480, 7.5"

// 3C
//#define D_Waveshare42YBW400300 // 400x300, 4.2"
//#define D_GDEQ042Z21    // 400x300, 4.2"
//#define D_HINK_E075A01  // 640x384, 7.5"
//#define D_GDEQ0583Z31   // 648x480, 5.83"
//#define D_GDEY075Z08    // 800x480, 7.5"

// 7C
//#define D_GDEY073D46    // 800x480, 7.3"

////////////
// Board
////////////

// https://www.laskakit.cz/laskakit-espink-esp32-e-paper-pcb-antenna/?variantId=12419
// Also ESPInk-42 all-in-one board

/* ---------------- Pinout of ESPink ------------ */
#ifdef ESPink
#define PIN_SS 5   // SS
#define PIN_DC 17  // D/C
#define PIN_RST 16 // RES
#define PIN_BUSY 4 // PIN_BUSY
#define ePaperPowerPin 2
#endif

#ifdef ES3ink
#define PIN_SS 9   // SS
#define PIN_DC 10  // D/C
#define PIN_RST 5  // RES
#define PIN_BUSY 6 // PIN_BUSY
#define ePaperPowerPin 48
#define enableBattery 40

#include <esp_adc_cal.h>
#include <soc/adc_channel.h>
esp_adc_cal_characteristics_t adc_cal;
#endif

#ifdef MakerBadge_revB
#define PIN_SS 41   // SS
#define PIN_DC 40  // D/C
#define PIN_RST 39 // RES
#define PIN_BUSY 42 // PIN_BUSY
#define ePaperPowerPin 16 // only version D and newer supports this feature
#endif

#ifdef MakerBadge_revD
#define PIN_SS 41   // SS
#define PIN_DC 40  // D/C
#define PIN_RST 39 // RES
#define PIN_BUSY 42 // PIN_BUSY
#define ePaperPowerPin 16 // only version D and newer supports this feature
#define enableBattery 14
#endif

//#define REMAP_SPI
#ifdef REMAP_SPI
#define PIN_SPI_CLK 13  // CLK
#define PIN_SPI_MISO 14 // unused
#define PIN_SPI_MOSI 12 // DIN
#define PIN_SPI_SS 15   // unused
#endif
/* ---------------------------------------------- */

// For LaskaKit ESP32-Dev kit use pins:
 /*
 SDI  = GPIO23
 SCL  = GPIO18
 CS   = GPIO5
 D/C  = GPIO17
 RES  = GPIO16
 PIN_BUSY = GPIO4
*/

///////////////////////
// ePaper libraries
///////////////////////

// 2 colors (Black and White)
#ifdef TYPE_BW
  #include <GxEPD2_BW.h>
  const char* defined_color_type = "BW";
#endif

// 3 colors (Black, White and Red/Yellow)
#ifdef TYPE_3C
  #include <GxEPD2_3C.h>
  const char* defined_color_type = "3C";
#endif

// 4 colors (Grayscale - Black, Darkgrey, Lightgrey, White) (https://github.com/ZinggJM/GxEPD2_4G)
#ifdef TYPE_GRAYSCALE
  #include "../lib/GxEPD2_4G/src/GxEPD2_4G_4G.h"
  #include "../lib/GxEPD2_4G/src/GxEPD2_4G_BW.h"
  const char* defined_color_type = "4G";
#endif

// 7 colors
#ifdef TYPE_7C
  #include <GxEPD2_7C.h>
  const char* defined_color_type = "7C";
#endif

/////////////////////////////////
// Use ePaper uncommented above
/////////////////////////////////

// Supported display classes can be found for example here:
// https://github.com/ZinggJM/GxEPD2/blob/master/examples/GxEPD2_Example/GxEPD2_display_selection.h
// If you need, you can get definition from there and then use it here 
// without uncommenting any particular display above

///////////////////////
// BW
///////////////////////

#ifdef D_B74
    GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(GxEPD2_213_B74(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

// GDEY0213B7 - BW, 122x250px, 2.13" 
#ifdef D_GDEY0213B7
    GxEPD2_BW<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT> display(GxEPD2_213_GDEY0213B74(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

// GDEY027T91 - BW, 176x246px, 2.7" 
#ifdef D_GDEY027T91
    GxEPD2_BW<GxEPD2_270_GDEY027T91, GxEPD2_270_GDEY027T91::HEIGHT> display(GxEPD2_270_GDEY027T91(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

// GDEW0154T8 - BW, 152x152px, 1.54" 
#ifdef D_GDEW0154T8
    GxEPD2_BW<GxEPD2_270_GDEY027T91, GxEPD2_270_GDEY027T91::HEIGHT> display(GxEPD2_270_GDEY027T91(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

// GDEW042T2 - BW, 400x300px, 4.2" 
#ifdef D_GDEW042T2
    GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT> display(GxEPD2_420(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

// GDEQ042T81 - BW, 400x300px, 4.2" 
#ifdef D_GDEQ042T81
    GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

// GDEQ0583T31 - BW, 648x480px, 5.83" 
#ifdef D_GDEQ0583T31
    GxEPD2_BW<GxEPD2_583_GDEQ0583T31, GxEPD2_583_GDEQ0583T31::HEIGHT> display(GxEPD2_583_GDEQ0583T31(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

// GDEW075T7 - BW, 800x480px, 7.5" 
#ifdef D_GDEW075T7
    GxEPD2_BW<GxEPD2_750, GxEPD2_750::HEIGHT> display(GxEPD2_750(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

// GDEQ102T90 - BW, 960x640px, 10.2" 
#ifdef D_GDEQ102T90
    GxEPD2_BW<GxEPD2_1160_T91, GxEPD2_1160_T91::HEIGHT / 2> display(GxEPD2_1160_T91(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

///////////////////////
// Grayscale
///////////////////////

// GDEW042T2_G - Grayscale, 400x300px, 4.2" 
#ifdef D_GDEW042T2_G
    GxEPD2_4G_4G<GxEPD2_420, GxEPD2_420::HEIGHT> display(GxEPD2_420(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

// GDEW075T7_G - Grayscale, 800x480px, 7.5" 
#ifdef D_GDEW075T7_G
    GxEPD2_4G_4G<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT / 2> display(GxEPD2_750_T7(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

///////////////////////
// 3C
///////////////////////

// Waveshare42YBW400300 - 3C, 400x300px, 4.2" 
#ifdef D_Waveshare42YBW400300
    GxEPD2_3C<GxEPD2_420c, GxEPD2_420c::HEIGHT> display(GxEPD2_420c(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

// GDEQ042Z21 - 3C, 400x300px, 4.2" 
#ifdef D_GDEQ042Z21
    GxEPD2_3C<GxEPD2_420c_Z21, GxEPD2_420c_Z21::HEIGHT> display(GxEPD2_420c_Z21(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

// HINK_E075A01 - 3C, 640x384px, 7.5" 
#ifdef D_HINK_E075A01
    GxEPD2_3C<GxEPD2_750c, GxEPD2_750c::HEIGHT> display(GxEPD2_750c(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

// GDEQ0583Z31 - 3C, 648x480px, 5.83" 
#ifdef D_GDEQ0583Z31
    GxEPD2_3C<GxEPD2_583c, GxEPD2_583c::HEIGHT> display(GxEPD2_583c(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

// GDEY075Z08 - 3C, 800x480px, 7.5" 
#ifdef D_GDEY075Z08
    GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT / 2> display(GxEPD2_750c_Z08(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

///////////////////////
// 7C
///////////////////////

// GDEY073D46 - 7C, 800x480px, 7.3" 
#ifdef D_GDEY073D46
    GxEPD2_7C<GxEPD2_730c_GDEY073D46, GxEPD2_730c_GDEY073D46::HEIGHT / 4> display(GxEPD2_730c_GDEY073D46(PIN_SS, PIN_DC, PIN_RST, PIN_BUSY));
#endif

///////////////////////////////////////////////
// That's all!
// Code of ZivyObraz follows
///////////////////////////////////////////////

////////////////////////////
// Library etc. includes
////////////////////////////

// WiFi
#include <WiFi.h>
#include <WiFiManager.h>

// SPI
#include "SPI.h"
// ADC reading
#include <ESP32AnalogRead.h>
// Font
#include "OpenSansSB_12px.h"

#ifdef REMAP_SPI
  SPIClass hspi(HSPI);
#endif

// SHT40 sensor
#ifdef SHT40
#include <Wire.h>
#include "Adafruit_SHT4x.h"
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
#endif

/* ---- ADC reading - indoor Battery voltage ---- */
#ifdef ES3ink
#define vBatPin ADC1_GPIO2_CHANNEL
#define dividerRatio 2.018

#elif defined MakerBadge_revB
ESP32AnalogRead adc;
#define vBatPin 9 
#define dividerRatio 1.5
#define BATT_V_CAL_SCALE 1.00

#elif defined MakerBadge_revD
digitalWrite(enableBattery, LOW);
ESP32AnalogRead adc;
#define vBatPin 9
#define dividerRatio 1.5
#define BATT_V_CAL_SCALE 1.05

#else
ESP32AnalogRead adc;
#define dividerRatio 1.769
#define vBatPin 34
#endif

/* ---- Server Zivy obraz ----------------------- */
const char* host = "cdn.zivyobraz.eu";
const char* firmware = "2.0";

/* ---------- Deepsleep time in minutes --------- */
uint64_t defaultDeepSleepTime = 2; // if there is a problem with loading images, 
                              // this time will be used as fallback to try again soon
uint64_t deepSleepTime = defaultDeepSleepTime; // actual sleep time in minutes, value is changed 
                                          // by what server suggest in response headers
/* ---------------------------------------------- */

/*-------------- ePaper resolution -------------- */
// Get display width from selected display class
#define DISPLAY_RESOLUTION_X display.epd2.WIDTH
#define DISPLAY_RESOLUTION_Y display.epd2.HEIGHT
/* ---------------------------------------------- */

/* variables */
int strength; // Wi-Fi signal strength
float d_volt; // indoor battery voltage
RTC_DATA_ATTR uint64_t timestamp = 0;
uint64_t timestampNow = 1; // initialize value for timestamp from server

int8_t getWifiStrength()
{
  strength = WiFi.RSSI();
  Serial.println("Wifi Strength: " + String(strength) + " dB"); 

  return strength;
}

uint8_t getBatteryVoltage()
{
#ifdef ES3ink

  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc_cal);
  adc1_config_channel_atten(vBatPin, ADC_ATTEN_DB_11);

  Serial.println("Reading battery on ES3ink board");
  bool readBattery = 0;
  do
  {
    digitalWrite(enableBattery, LOW);
    uint32_t raw = adc1_get_raw(vBatPin);
    Serial.println(raw);
    uint32_t millivolts = esp_adc_cal_raw_to_voltage(raw, &adc_cal);
    Serial.println(millivolts);
    const uint32_t upper_divider = 1000;
    const uint32_t lower_divider = 1000;
    d_volt = (float)(upper_divider + lower_divider) / lower_divider / 1000 * millivolts;
    readBattery = 1;
    digitalWrite(enableBattery, LOW);
  } while (readBattery != 1);

  Serial.println("Battery voltage: " + String(d_volt) + " V");

  return d_volt;

#elif defined MakerBadge_revB
  adc.attach(vBatPin);
  d_volt = (BATT_V_CAL_SCALE*2.0*(2.50*batt_adc/4096)); // d_volt = adc.readVoltage() * dividerRatio;
  Serial.println("Battery voltage: " + String(d_volt) + " V");

  return d_volt;

#elif defined MakerBadge_revD
  adc.attach(vBatPin);
  d_volt = (BATT_V_CAL_SCALE*2.0*(2.50*batt_adc/4096)); // d_volt = adc.readVoltage() * dividerRatio;
  Serial.println("Battery voltage: " + String(d_volt) + " V");

  return d_volt;

#else
  // attach ADC input
  adc.attach(vBatPin);
  // battery voltage measurement
  d_volt = adc.readVoltage() * dividerRatio;
  Serial.println("Battery voltage: " + String(d_volt) + " V");

  return d_volt;
#endif
}

void displayInit()
{
  #ifdef REMAP_SPI
    // only CLK and MOSI are important for EPD
    hspi.begin(PIN_SPI_CLK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_SS);  // swap pins
    display.epd2.selectSPI(hspi, SPISettings(4000000, MSBFIRST, SPI_MODE0));
  #endif

  #ifdef ES3ink
    display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  #else
    display.init();
  #endif
  display.setRotation(0);
  display.fillScreen(GxEPD_WHITE);  // white background
  display.setTextColor(GxEPD_BLACK);  // black font
}

void WiFiInit()
{
  // Connecting to WiFi
  Serial.println();
  Serial.print("Connecting... ");
  //WiFi.begin(ssid, pass);
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setWiFiAutoReconnect(true);
  wm.setConnectRetries(3);
  wm.setDarkMode(true);
  wm.setConnectTimeout(3);
  wm.setSaveConnectTimeout(3);

  // reset settings - wipe stored credentials for testing
  //wm.resetSettings();

  wm.setConfigPortalTimeout(300);   // set portal time to 5 min, then sleep/try again.
  bool res;
  // Set network name to wi-fi mac address
  String hostname = "INK_";
  hostname += WiFi.macAddress();
  // Replace colon with nothing
  hostname.replace(":", "");

  res = wm.autoConnect(hostname.c_str() ,"zivyobraz"); // password protected ap

  // Check if Wi-Fi is configured or connection to AP failed - show on ePaper
  if(!wm.getWiFiIsSaved() or !res) {
      displayInit();
      pinMode(ePaperPowerPin, OUTPUT); 
      #ifdef ES3ink
          digitalWrite(ePaperPowerPin, LOW);
      #else
          digitalWrite(ePaperPowerPin, HIGH);
      #endif
      delay(500);

      display.setFont(&OpenSansSB_12px);
      display.setCursor(10, 20);
      display.print("No Wi-Fi configuration/connection.");
      display.setCursor(10, 40);
      display.print("Configuration portal has been started!");
      display.setCursor(10, 60);
      display.print("Connect to AP and (re)configure Wi-Fi connection:");
      display.setCursor((DISPLAY_RESOLUTION_X / 2) - 70, 100);
      display.print(hostname);

      display.display(false); // update screen
      digitalWrite(ePaperPowerPin, LOW);

      timestamp = 0; // set timestamp to 0 to force update because we changed screen to this info
  }

  if(!res) {
      Serial.println("Failed to connect to Wi-Fi, starting portal");
  } 
  else {
        //if you get here you have connected to the WiFi    
        Serial.println("Wi-Fi connected successfully");
  }
}

uint32_t skip(WiFiClient& client, int32_t bytes)
{
  int32_t remain = bytes;
  uint32_t start = millis();
  while ((client.connected() || client.available()) && (remain > 0))
  {
    if (client.available())
    {
      client.read();
      remain--;
    }
    else delay(1);
    if (millis() - start > 2000) break; // don't hang forever
  }
  return bytes - remain;
}

uint32_t read8n(WiFiClient& client, uint8_t* buffer, int32_t bytes)
{
  int32_t remain = bytes;
  uint32_t start = millis();
  while ((client.connected() || client.available()) && (remain > 0))
  {
    if (client.available())
    {
      int16_t v = client.read();
      *buffer++ = uint8_t(v);
      remain--;
    }
    else delay(1);
    if (millis() - start > 2000) break; // don't hang forever
  }
  return bytes - remain;
}

uint16_t read16(WiFiClient& client)
{
  // BMP data is stored little-endian, same as Arduino.
  uint16_t result;
  ((uint8_t *)&result)[0] = client.read(); // LSB
  ((uint8_t *)&result)[1] = client.read(); // MSB
  return result;
}

uint32_t read32(WiFiClient& client)
{
  // BMP data is stored little-endian, same as Arduino.
  uint32_t result;
  ((uint8_t *)&result)[0] = client.read(); // LSB
  ((uint8_t *)&result)[1] = client.read();
  ((uint8_t *)&result)[2] = client.read();
  ((uint8_t *)&result)[3] = client.read(); // MSB
  return result;
}

bool checkForNewTimestampOnServer()
{
  // Connect to the HOST and read data via GET method
  WiFiClient client; // Use WiFiClient class to create TCP connections
  bool connection_ok = false;

  // Make an url
  String url = "index.php?mac=" + WiFi.macAddress() + "&timestamp_check=1&rssi=" + String(strength) + "&v=" + String(d_volt) + "&x=" + String(DISPLAY_RESOLUTION_X) + "&y=" + String(DISPLAY_RESOLUTION_Y) + "&c=" + String(defined_color_type) + "&fw=" + String(firmware);

  ////////////////////////////////////////
  // Measuring temperature and humidity?
  ////////////////////////////////////////

  #ifdef SHT40
  // LaskaKit ESPInk 2.5 needst to power up uSup
  pinMode(ePaperPowerPin, OUTPUT); 
  digitalWrite(ePaperPowerPin, HIGH);
  delay(50);

  if (! sht4.begin()) 
  {
    Serial.println("SHT4x not found");
    Serial.println("Check the connection");
    delay(1000);
  }
  else
  {
    sht4.setPrecision(SHT4X_HIGH_PRECISION); // highest resolution
    sht4.setHeater(SHT4X_NO_HEATER); // no heater
    float teplota = 0.0;
    int vlhkost = 0;

    sensors_event_t humidity, temp; // temperature and humidity variables
    sht4.getEvent(&humidity, &temp);

    teplota = temp.temperature;
    vlhkost = humidity.relative_humidity;

    url += "&temp=" + String(teplota) + "&hum=" + String(vlhkost);
  }

  // Power down for now
  digitalWrite(ePaperPowerPin, LOW);
  #endif

  ////////////////////////////////////////

  Serial.print("connecting to "); Serial.println(host);

  for (int client_reconnect = 0; client_reconnect < 3; client_reconnect++)
  {
    if (!client.connect(host, 80))
    {
      Serial.println("connection failed");
      if(client_reconnect == 2)
      {
        deepSleepTime = defaultDeepSleepTime;
        delay(200);
      }
    }
  }

  Serial.print("requesting URL: ");
  Serial.println(String("http://") + host + "/" + url);
  client.print(String("GET ") + "/" + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  Serial.println("request sent");

  // Workaroud for timeout
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 10000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      deepSleepTime = defaultDeepSleepTime;
      return false;
    }
  }

  int gotTimestamp = 0;

  while (client.connected())
  {
    String line = client.readStringUntil('\n');

    // If line starts with "Timestamp", put it into the timestamp variable
    if (line.startsWith("Timestamp"))
    {
      gotTimestamp = 1;
      timestampNow = line.substring(11).toInt();
      Serial.print("Timestamp now: ");
      Serial.println(timestampNow);
    }

    // Let's try to get info about how long to go to deep sleep
    if(line.startsWith("Sleep"))
    {
      uint64_t sleep = line.substring(7).toInt();
      deepSleepTime = sleep;
      Serial.print("Sleep: ");
      Serial.println(sleep);
    }

    if (!connection_ok)
    {
      connection_ok = line.startsWith("HTTP/1.1 200 OK");
      if (connection_ok) Serial.println(line);
      //if (!connection_ok) Serial.println(line);
    }
    if (!connection_ok) Serial.println(line);
    //Serial.println(line);
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
  }

  if (!connection_ok)
  { 
    deepSleepTime = defaultDeepSleepTime;
    return false;
  }

  if (gotTimestamp == 1 && timestampNow == timestamp)
  {
    Serial.print("No screen reload, because we already are at current timestamp: "); 
    Serial.println(timestamp);
    return false;
  }
  else
  {
    // Set timestamp to actual one
    timestamp = timestampNow;
    return true;
  }
}

void readBitmapData()
{
  // Connect to the HOST and read data via GET method
  WiFiClient client; // Use WiFiClient class to create TCP connections

  // Let's read bitmap
  static const uint16_t input_buffer_pixels = 800; // may affect performance
  static const uint16_t max_row_width = 1872; // for up to 7.8" display 1872x1404
  static const uint16_t max_palette_pixels = 256; // for depth <= 8

  int16_t x = display.width() - DISPLAY_RESOLUTION_X;
  int16_t y = display.height() - DISPLAY_RESOLUTION_Y;

  uint8_t input_buffer[3 * input_buffer_pixels]; // up to depth 24
  uint8_t output_row_mono_buffer[max_row_width / 8]; // buffer for at least one row of b/w bits
  uint8_t output_row_color_buffer[max_row_width / 8]; // buffer for at least one row of color bits
  uint8_t mono_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 b/w
  uint8_t color_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 c/w
  uint16_t rgb_palette_buffer[max_palette_pixels]; // palette buffer for depth <= 8 for buffered graphics, needed for 7-color display

  // Defaults - also for BW
  bool with_color = false;
  bool has_multicolors = false;
  bool grayscale = false;

  #ifdef TYPE_GRAYSCALE
    with_color = true;
    has_multicolors = false;
    grayscale = true;
  #endif

  #ifdef TYPE_7C
    with_color = true;
    has_multicolors = true;
    grayscale = false;
  #endif

  #ifdef TYPE_3C
    with_color = true;
    has_multicolors = false;
    grayscale = false;
  #endif

  bool connection_ok = false;
  bool valid = false; // valid format to be handled
  bool flip = true; // bitmap is stored bottom-to-top

  bool whitish = false;
  bool lightgrey = false;
  bool darkgrey = false;
  bool colored = false;

  uint32_t startTime = millis();
  if ((x >= display.width()) || (y >= display.height())) return;
  Serial.print("connecting to "); Serial.println(host);
  // Let's try twice
  for (int client_reconnect = 0; client_reconnect < 3; client_reconnect++)
  {
    if (!client.connect(host, 80))
    {
      Serial.println("connection failed");
      if(client_reconnect == 2)
      {
        deepSleepTime = defaultDeepSleepTime;
        return;
      }
      delay(200);
    }
  }

  // Make an url
  String url = "index.php?mac=" + WiFi.macAddress() + "&rssi=" + String(strength) + "&v=" + String(d_volt) + "&x=" + String(DISPLAY_RESOLUTION_X) + "&y=" + String(DISPLAY_RESOLUTION_Y) + "&c=" + String(defined_color_type) + "&fw=" + String(firmware);
  Serial.print("requesting URL: ");
  Serial.println(String("http://") + host + "/" + url);
  client.print(String("GET ") + "/" + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  Serial.println("request sent");

  // Workaroud for timeout
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 10000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  int gotTimestamp = 0;

  while (client.connected())
  {
    String line = client.readStringUntil('\n');

    if (!connection_ok)
    {
      connection_ok = line.startsWith("HTTP/1.1 200 OK");
      if (connection_ok) Serial.println(line);
      //if (!connection_ok) Serial.println(line);
    }
    if (!connection_ok) Serial.println(line);
    //Serial.println(line);
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
  }

  // Is there a problem? Fallback to default deep sleep time to try again soon
  if (!connection_ok)
  { 
    deepSleepTime = defaultDeepSleepTime;
    return;
  }

    // For debug purposes - print out the whole response
    /*
    Serial.println("Byte by byte:");

    while (client.connected() || client.available()) {
      if (client.available()) {
        char c = client.read();  // Read one byte
        Serial.print(c);         // Print the byte to the serial monitor
      }
    }
    client.stop();
    /* */

  // Parse header
  uint16_t header = read16(client);
  Serial.print("Header "); Serial.println(header, HEX);
 
  if (header == 0x4D42) // BMP signature
  {
    //#include <pgmspace.h>
    uint32_t fileSize = read32(client);
    uint32_t creatorBytes = read32(client); (void)creatorBytes; //unused
    uint32_t imageOffset = read32(client); // Start of image data
    uint32_t headerSize = read32(client);
    uint32_t width  = read32(client);
    int32_t height = (int32_t) read32(client);
    uint16_t planes = read16(client);
    uint16_t depth = read16(client); // bits per pixel
    uint32_t format = read32(client);
    uint32_t bytes_read = 7 * 4 + 3 * 2; // read so far
    if ((planes == 1) && ((format == 0) || (format == 3))) // uncompressed is handled, 565 also
    {
      Serial.print("File size: "); Serial.println(fileSize);
      Serial.print("Image Offset: "); Serial.println(imageOffset);
      Serial.print("Header size: "); Serial.println(headerSize);
      Serial.print("Bit Depth: "); Serial.println(depth);
      Serial.print("Image size: ");
      Serial.print(width);
      Serial.print('x');
      Serial.println(height);
      // BMP rows are padded (if needed) to 4-byte boundary
      uint32_t rowSize = (width * depth / 8 + 3) & ~3;
      if (depth < 8) rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;
      if (height < 0)
      {
        height = -height;
        flip = false;
      }
      uint16_t w = width;
      uint16_t h = height;
      if ((x + w - 1) >= display.width())  w = display.width()  - x;
      if ((y + h - 1) >= display.height()) h = display.height() - y;
      
      //if (w <= max_row_width) // handle with direct drawing
      {
        valid = true;
        uint8_t bitmask = 0xFF;
        uint8_t bitshift = 8 - depth;
        uint16_t red, green, blue, rowToShow;
        whitish = false;
        lightgrey = false;
        darkgrey = false;
        colored = false;
        if (depth == 1) with_color = false;
        if (depth <= 8)
        {
          if (depth < 8) bitmask >>= depth;
          //bytes_read += skip(client, 54 - bytes_read); //palette is always @ 54
          bytes_read += skip(client, imageOffset - (4 << depth) - bytes_read); // 54 for regular, diff for colorsimportant
          for (uint16_t pn = 0; pn < (1 << depth); pn++)
          {
            blue  = client.read();
            green = client.read();
            red   = client.read();
            client.read();
            bytes_read += 4;
            whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
            colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
            if (0 == pn % 8) mono_palette_buffer[pn / 8] = 0;
            mono_palette_buffer[pn / 8] |= whitish << pn % 8;
            if (0 == pn % 8) color_palette_buffer[pn / 8] = 0;
            color_palette_buffer[pn / 8] |= colored << pn % 8;
            //Serial.print("0x00"); Serial.print(red, HEX); Serial.print(green, HEX); Serial.print(blue, HEX);
            //Serial.print(" : "); Serial.print(whitish); Serial.print(", "); Serial.println(colored);
            rgb_palette_buffer[pn] = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
          }
        }

        uint32_t rowPosition = flip ? imageOffset + (height - h) * rowSize : imageOffset;
        //Serial.print("skip "); Serial.println(rowPosition - bytes_read);
        bytes_read += skip(client, rowPosition - bytes_read);

        for (uint16_t row = 0; row < h; row++, rowPosition += rowSize) // for each line
        {
          if (!connection_ok || !(client.connected() || client.available())) break;
          delay(1); // yield() to avoid WDT
          uint32_t in_remain = rowSize;
          uint32_t in_idx = 0;
          uint32_t in_bytes = 0;
          uint8_t in_byte = 0; // for depth <= 8
          uint8_t in_bits = 0; // for depth <= 8
          uint16_t color = GxEPD_WHITE;
          for (uint16_t col = 0; col < w; col++) // for each pixel
          {
            yield();
            if (!connection_ok || !(client.connected() || client.available())) break;
            // Time to read more pixel data?
            if (in_idx >= in_bytes) // ok, exact match for 24bit also (size IS multiple of 3)
            {
              uint32_t get = in_remain > sizeof(input_buffer) ? sizeof(input_buffer) : in_remain;
              uint32_t got = read8n(client, input_buffer, get);
              while ((got < get) && connection_ok)
              {
                //Serial.print("got "); Serial.print(got); Serial.print(" < "); Serial.print(get); Serial.print(" @ "); Serial.println(bytes_read);
                uint32_t gotmore = read8n(client, input_buffer + got, get - got);
                got += gotmore;
                connection_ok = gotmore > 0;
              }
              in_bytes = got;
              in_remain -= got;
              bytes_read += got;
            }
            if (!connection_ok)
            {
              Serial.print("Error: got no more after "); Serial.print(bytes_read); Serial.println(" bytes read!");
              break;
            }

            whitish = false;
            lightgrey = false;
            darkgrey = false;
            colored = false;
            switch (depth)
            {
              case 32:
                blue = input_buffer[in_idx++];
                green = input_buffer[in_idx++];
                red = input_buffer[in_idx++];
                in_idx++; // skip alpha
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                lightgrey = with_color ? ((red > 0x60) && (green > 0x60) && (blue > 0x60)) : ((red + green + blue) > 3 * 0x60); // lightgrey
                darkgrey = with_color ? ((red > 0x40) && (green > 0x40) && (blue > 0x40)) : ((red + green + blue) > 3 * 0x40); // darkgrey
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
                color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
                break;
              case 24:
                blue = input_buffer[in_idx++];
                green = input_buffer[in_idx++];
                red = input_buffer[in_idx++];
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                lightgrey = with_color ? ((red > 0x60) && (green > 0x60) && (blue > 0x60)) : ((red + green + blue) > 3 * 0x60); // lightgrey
                darkgrey = with_color ? ((red > 0x40) && (green > 0x40) && (blue > 0x40)) : ((red + green + blue) > 3 * 0x40); // darkgrey
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
                color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
                break;
              case 16:
                {
                  uint8_t lsb = input_buffer[in_idx++];
                  uint8_t msb = input_buffer[in_idx++];
                  if (format == 0) // 555
                  {
                    blue  = (lsb & 0x1F) << 3;
                    green = ((msb & 0x03) << 6) | ((lsb & 0xE0) >> 2);
                    red   = (msb & 0x7C) << 1;
                    color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
                  }
                  else // 565
                  {
                    blue  = (lsb & 0x1F) << 3;
                    green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
                    red   = (msb & 0xF8);
                    color = (msb << 8) | lsb;
                  }
                  whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                  lightgrey = with_color ? ((red > 0x60) && (green > 0x60) && (blue > 0x60)) : ((red + green + blue) > 3 * 0x60); // lightgrey
                  darkgrey = with_color ? ((red > 0x40) && (green > 0x40) && (blue > 0x40)) : ((red + green + blue) > 3 * 0x40); // darkgrey
                  colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
                }
                break;
              case 1:
              case 2:
              case 4:
              case 8:
                {
                  if (0 == in_bits)
                  {
                    in_byte = input_buffer[in_idx++];
                    in_bits = 8;
                  }
                  uint16_t pn = (in_byte >> bitshift) & bitmask;
                  whitish = mono_palette_buffer[pn / 8] & (0x1 << pn % 8);
                  colored = color_palette_buffer[pn / 8] & (0x1 << pn % 8);
                  in_byte <<= depth;
                  in_bits -= depth;
                  color = rgb_palette_buffer[pn];

                  if(grayscale)
                  {
                    switch(pn){
                      case 1:
                        lightgrey = true;
                      break;
                      case 2:
                        darkgrey = true;
                      break;
                      case 3:
                        darkgrey = true;
                      break;
                      case 4:
                        whitish = true;
                      break;
                    }
                  }
                }
                break;
            }
            if (with_color && has_multicolors)
            {
              // keep color
            }
            else if (whitish)
            {
              color = GxEPD_WHITE;
            }
            else if (grayscale && lightgrey)
            {
              color = GxEPD_LIGHTGREY;
            }
            else if (grayscale && darkgrey)
            {
              color = GxEPD_DARKGREY;
            }
            else if (colored && with_color)
            {
              color = GxEPD_COLORED;
            }
            else
            {
              color = GxEPD_BLACK;
            }

            uint16_t yrow = y + (flip ? h - row - 1 : row);
            display.drawPixel(x + col, yrow, color);
          } // end col
        } // end row
      } // end block

      Serial.print("bytes read "); Serial.println(bytes_read);
    }

  }
  else if(header == 0x315A || header == 0x325A) // ZivyObraz RLE data Z1 or Z2
  {
    // Z1 - 1 byte for color, 1 byte for number of repetition
    // Z2 - 2 bits for color, 6 bits for number of repetition
    if(header == 0x315A) Serial.println("Got format Z1, processing");
    if(header == 0x325A) Serial.println("Got format Z2, processing");

    uint32_t bytes_read = 2; // read so far
    uint16_t w = display.width();
    uint16_t h = display.height();
    uint8_t pixel_color, count, compressed;
    uint16_t color;
    valid = true;

    uint16_t color2 = GxEPD_LIGHTGREY;
    uint16_t color3 = GxEPD_DARKGREY;

    #ifdef TYPE_3C
      color2 = GxEPD_RED;
      color3 = GxEPD_YELLOW;
    #endif

    #ifdef TYPE_7C
      color2 = GxEPD_RED;
      color3 = GxEPD_YELLOW;
    #endif

    for (uint16_t row = 0; row < h; row++) // for each line
    {
      if (!connection_ok || !(client.connected() || client.available())) break;
      delay(1); // yield() to avoid WDT

      for (uint16_t col = 0; col < w; col++) // for each pixel
      {
        yield();

        if (!connection_ok)
        {
          Serial.print("Error: got no more after "); Serial.print(bytes_read); Serial.println(" bytes read!");
          break;
        }

        if (!(client.connected() || client.available()))
        {
          Serial.println("Client got disconnected after bytes:");
          Serial.println(bytes_read);
          break;
        }

        // Z1
        if(header == 0x315A)
        {
          pixel_color = client.read();
          count = client.read();
          bytes_read += 2;
        } else if (header == 0x325A) {
          // Z2
          compressed = client.read();
          count = compressed & 0b00111111;
          pixel_color = (compressed & 0b11000000) >> 6;
          bytes_read++;
        }

        switch(pixel_color)
        {
          case 0x0:
            color = GxEPD_WHITE;
          break;
          case 0x1:
            color = GxEPD_BLACK;
          break;
          case 0x2:
            color = color2;
          break;
          case 0x3:
            color = color3;
          break;
          #ifdef TYPE_7C
            case 0x4:
              color = GxEPD_GREEN;
            break;
            case 0x5:
              color = GxEPD_BLUE;
            break;
            case 0x6:
              color = GxEPD_ORANGE;
            break;
          #endif

        }

        // Debug
        /*
        if(bytes_read < 20)
        {
          Serial.print("count: "); Serial.print(count); Serial.print(" pixel: "); Serial.println(color);
        }
        /* */

        for(uint8_t i = 0; i < count - 1; i++)
        {
          display.drawPixel(col, row, color);

          if(count > 1)
          {
            col++;

            if(col == w)
            {
              col = 0;
              row++;
            }
          }
        }

        display.drawPixel(col, row, color);
      } // end col
    } // end row

    Serial.print("bytes read "); Serial.println(bytes_read);
  }

    Serial.print("loaded in "); Serial.print(millis() - startTime); Serial.println(" ms");

    client.stop();
    if (!valid)
    {
      Serial.print("Format not handled, got: ");
      Serial.println(header);
      deepSleepTime = defaultDeepSleepTime;
      timestamp = 0;
    }
}

void setup()
{
  Serial.begin(115200);

  #ifdef ES3ink
    pinMode(enableBattery, OUTPUT);
  #endif

  // Battery voltage measurement
  getBatteryVoltage();

  // ePaper init
  displayInit();

  // Wifi init
  WiFiInit();

  // WiFi strength - so you will know how good your signal is
  getWifiStrength();

  // Do we need to update the screen?
  if(checkForNewTimestampOnServer())
  {
    pinMode(ePaperPowerPin, OUTPUT); 
    #ifdef ES3ink
        digitalWrite(ePaperPowerPin, LOW);
    #else
        digitalWrite(ePaperPowerPin, HIGH);
    #endif
    delay(500);

    // Get that lovely bitmap and put it on your gorgeous grayscale ePaper screen!

    // If you can't use whole display at once, there will be multiple pages and therefore 
    // requests and downloads of one bitmap from server, since you have to always write whole image
    display.setFullWindow();
    display.firstPage();
    do
    {
      readBitmapData();
    }
    while (display.nextPage());

    // Disable power supply for ePaper
    #ifdef ES3ink
        digitalWrite(ePaperPowerPin, HIGH);
    #else
        digitalWrite(ePaperPowerPin, LOW);
    #endif
  }

  // Deep sleep mode
  Serial.print("Going to sleep now for (minutes): ");
  Serial.println(deepSleepTime);
  esp_sleep_enable_timer_wakeup(deepSleepTime * 60 * 1000000);
  delay(200);
  esp_deep_sleep_start();
}

void loop()
{

}
