#include "board.h"

#include "display.h"
#include "sensor.h"

// M5Stack CoreInk
#ifdef M5StackCoreInk
  #include <M5CoreInk.h>
#endif

// TWI/I2C library
#include <Wire.h>

#ifdef ESPink_V3
  #include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>
static SFE_MAX1704X lipo(MAX1704X_MAX17048);
#endif

#if (defined ESP32S3Adapter) || (defined ES3ink) || (defined SVERIO_PAPERBOARD_SPI)
  #include <esp_adc_cal.h>
  #include <soc/adc_channel.h>
#endif

namespace Board
{

void setupHW()
{
#ifdef ES3ink
  // Battery voltage reading via PMOS switch with series capacitor to gate.
  digitalWrite(enableBattery, HIGH);
  pinMode(enableBattery, OUTPUT);
  pinMode(RGBledPowerPin, OUTPUT);
  digitalWrite(RGBledPowerPin, HIGH);
  Display::pixelInit();
#endif

#ifdef M5StackCoreInk
  M5.begin(false, false, true);
  Display::initM5();
  M5.update();
#elif !defined SEEEDSTUDIO_RETERMINAL
  pinMode(ePaperPowerPin, OUTPUT);
#endif

#if (defined SVERIO_PAPERBOARD_SPI) || (defined SEEEDSTUDIO_RETERMINAL)
  pinMode(enableBattery, OUTPUT);
  digitalWrite(enableBattery, LOW);
#endif

#ifdef CROWPANEL_ESP32S3_579
  pinMode(ePaperPowerPin, OUTPUT);
  setEPaperPowerOn(true);
  delay(50);
#endif

#ifdef SENSOR
  Sensor::getInstance().init();
#endif

  // Initialize display
  Display::init();
}

void setEPaperPowerOn(bool on)
{
  // use HIGH/LOW notation for better readability
#if (defined ES3ink) || (defined MakerBadge_revD) || (defined SVERIO_PAPERBOARD_SPI)
  digitalWrite(ePaperPowerPin, on ? LOW : HIGH);
#elif (!defined M5StackCoreInk) && (!defined SEEEDSTUDIO_RETERMINAL)
  digitalWrite(ePaperPowerPin, on ? HIGH : LOW);
#endif
}

void enterDeepSleepMode(uint64_t sleepDuration)
{
  // Enter deep sleep
#ifdef M5StackCoreInk
  Display::powerOffM5();
  M5.shutdown(sleepDuration);
#else
  esp_sleep_enable_timer_wakeup(sleepDuration * 1000000ULL);
  // Configure button as additional wake source (wake on LOW level - active low button)
  #if defined(EXT_BUTTON) && !defined(ESPink_V35)
  esp_sleep_enable_ext1_wakeup(1ULL << EXT_BUTTON, ESP_EXT1_WAKEUP_ANY_LOW);
  #endif
  delay(100);
  esp_deep_sleep_start();
#endif
}

float getBatteryVoltage()
{
  float volt;

#ifdef ESPink_V3
  Serial.println("[BATTERY] Readingon ESPink V3 board");

  setEPaperPowerOn(true);
  pinMode(PIN_ALERT, INPUT_PULLUP);

  delay(100);

  Wire.begin(PIN_SDA, PIN_SCL);

  lipo.begin();

  // lipo.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  // Read and print the reset indicator
  Serial.print(F("[BATTERY] Reset Indicator was: "));
  bool RI = lipo.isReset(true); // Read the RI flag and clear it automatically if it is set
  Serial.println(RI);           // Print the RI
  // If RI was set, check it is now clear
  if (RI)
  {
    Serial.print(F("[BATTERY] Reset Indicator is now: "));
    RI = lipo.isReset(); // Read the RI flag
    Serial.println(RI);  // Print the RI
  }

  lipo.setThreshold(1);         // Set alert threshold to just 1% - we don't want to trigger the alert
  lipo.setVALRTMax((float)4.3); // Set high voltage threshold (Volts)
  lipo.setVALRTMin((float)2.9); // Set low voltage threshold (Volts)

  volt = (float)lipo.getVoltage();
  // percentage could be read like this:
  // lipo.getSOC();
  // Serial.println("[BATTERY] Percentage: " + String(lipo.getSOC(), 2) + " %");

  lipo.clearAlert();
  lipo.enableHibernate();

  setEPaperPowerOn(false);

#elif defined ES3ink
  esp_adc_cal_characteristics_t adc_cal;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 0, &adc_cal);
  adc1_config_channel_atten(vBatPin, ADC_ATTEN_DB_12);

  Serial.println("[BATTERY] Reading on ES3ink board");

  digitalWrite(enableBattery, LOW);
  uint32_t raw = adc1_get_raw(vBatPin);
  // Serial.println(raw);
  uint32_t millivolts = esp_adc_cal_raw_to_voltage(raw, &adc_cal);
  // Serial.println(millivolts);
  const uint32_t upper_divider = 1000;
  const uint32_t lower_divider = 1000;
  volt = (float)(upper_divider + lower_divider) / lower_divider / 1000 * millivolts;
  digitalWrite(enableBattery, HIGH);

#elif defined ESP32S3Adapter
  Serial.println("[BATTERY] Reading on ESP32-S3 DEVKIT board");
  // attach ADC input
  volt = (analogReadMilliVolts(vBatPin) * dividerRatio / 1000);

#elif defined M5StackCoreInk
  analogSetPinAttenuation(vBatPin, ADC_11db);
  esp_adc_cal_characteristics_t *adc_chars =
    (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 3600, adc_chars);
  uint16_t ADCValue = analogRead(vBatPin);

  uint32_t BatVolmV = esp_adc_cal_raw_to_voltage(ADCValue, adc_chars);
  volt = float(BatVolmV) * 25.1 / 5.1 / 1000;
  free(adc_chars);

#elif defined MakerBadge_revB
  volt = (BATT_V_CAL_SCALE * 2.0 * (2.50 * analogRead(vBatPin) / 8192));

#elif defined MakerBadge_revD
  // Borrowed from @Yourigh
  // Battery voltage reading
  // can be read right after High->Low transition of IO_BAT_meas_disable
  // Here, pin should not go LOW, so intentionally digitalWrite called as first.
  // First write output register (PORTx) then activate output direction (DDRx). Pin will go from highZ(sleep) to HIGH
  // without LOW pulse.
  digitalWrite(enableBattery, HIGH);
  pinMode(enableBattery, OUTPUT);

  digitalWrite(enableBattery, LOW);
  delayMicroseconds(150);
  volt = (BATT_V_CAL_SCALE * 2.0 * (2.50 * analogRead(vBatPin) / 8192));
  digitalWrite(enableBattery, HIGH);

#elif defined SVERIO_PAPERBOARD_SPI
  // Battery measurement with calibrated ADC on SVERIO SPI Paperboard
  esp_adc_cal_characteristics_t adc_cal;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 0, &adc_cal);
  adc1_config_channel_atten(vBatPin, ADC_ATTEN_DB_12);

  // Enable the measurement path via PMOS gate
  digitalWrite(enableBattery, HIGH);
  delay(200);

  uint32_t raw = adc1_get_raw(vBatPin);
  uint32_t millivolts = esp_adc_cal_raw_to_voltage(raw, &adc_cal);

  digitalWrite(enableBattery, LOW);

  const uint32_t upper_divider = 1000;
  const uint32_t lower_divider = 1000;
  volt = (float)(upper_divider + lower_divider) / lower_divider / 1000 * millivolts;
  volt = volt * dividerRatio;

#elif defined TTGO_T5_v23
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)ADC_UNIT_1, (adc_atten_t)ADC_ATTEN_DB_2_5,
                                                          (adc_bits_width_t)ADC_WIDTH_BIT_12, 1100, &adc_chars);

  float measurement = (float)analogRead(vBatPin);
  volt = (float)(measurement / 4095.0) * 7.05;

#elif (defined SEEEDSTUDIO_XIAO_ESP32C3) || (defined CROWPANEL_ESP32S3_579) || (defined CROWPANEL_ESP32S3_42) ||       \
  (defined CROWPANEL_ESP32S3_213)
  volt = (float)0;

#elif defined SEEEDSTUDIO_XIAO_EDDB_ESP32S3
  digitalWrite(enableBattery, HIGH);
  pinMode(enableBattery, OUTPUT);
  delay(8); // slow tON time TPS22916C. 6500us typical for 1V. Set 8ms as margin.
  volt = ((float)analogReadMilliVolts(vBatPin) * dividerRatio) / 1000;
  digitalWrite(enableBattery, LOW);
  pinMode(enableBattery, INPUT);

#elif defined SEEEDSTUDIO_RETERMINAL
  Serial.println("[BATTERY] Reading on SeeedStudio reTerminal board");
  // Enable battery voltage measurement circuit via GPIO21
  digitalWrite(enableBattery, HIGH);
  pinMode(enableBattery, OUTPUT);
  delay(10); // Allow measurement circuit to stabilize
  volt = ((float)analogReadMilliVolts(vBatPin) * dividerRatio) / 1000;
  digitalWrite(enableBattery, LOW);
  pinMode(enableBattery, INPUT);

#else
  volt = (analogReadMilliVolts(vBatPin) * dividerRatio / 1000);
#endif

  Serial.println("[BATTERY] Voltage: " + String(volt) + " V");
  return volt;
}

unsigned long checkButtonPressDuration()
{
#ifdef EXT_BUTTON
  pinMode(EXT_BUTTON, INPUT_PULLUP);

  // Check if button is pressed (LOW = pressed with pull-up)
  if (digitalRead(EXT_BUTTON) == HIGH)
    return 0; // Button not pressed

  Serial.println("[BUTTON] Press detected at boot, measuring duration...");

  unsigned long pressStart = millis();
  const unsigned long maxWaitTime = 10000;

  // Wait while button is pressed (LOW)
  while (digitalRead(EXT_BUTTON) == LOW)
  {
    delay(50);

    if (millis() - pressStart > maxWaitTime)
      break;
  }

  unsigned long pressDuration = millis() - pressStart;
  Serial.println("[BUTTON] Press duration: " + String(pressDuration) + " ms");
  return pressDuration;
#else
  // EXT_BUTTON not defined for this board
  return 0;
#endif
}

float getCPUTemperature()
{
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3
  return temperatureRead();
#else
  return 0.0f;
#endif
}

ResetReason getResetReason()
{
  esp_reset_reason_t reason = esp_reset_reason();

  switch (reason)
  {
    case ESP_RST_POWERON:
      return ResetReason::POWERON;
    case ESP_RST_EXT:
      return ResetReason::EXT;
    case ESP_RST_SW:
      return ResetReason::SW;
    case ESP_RST_PANIC:
      return ResetReason::PANIC;
    case ESP_RST_INT_WDT:
      return ResetReason::INT_WDT;
    case ESP_RST_TASK_WDT:
      return ResetReason::TASK_WDT;
    case ESP_RST_WDT:
      return ResetReason::WDT;
    case ESP_RST_DEEPSLEEP:
      return ResetReason::DEEPSLEEP;
    case ESP_RST_BROWNOUT:
      return ResetReason::BROWNOUT;
    case ESP_RST_SDIO:
      return ResetReason::SDIO;
    default:
      return ResetReason::UNKNOWN;
  }
}

const char *getResetReasonString()
{
  esp_reset_reason_t reason = esp_reset_reason();
  switch (reason)
  {
    case ESP_RST_POWERON:
      return "poweron";
    case ESP_RST_EXT:
      return "external";
    case ESP_RST_SW:
      return "software";
    case ESP_RST_PANIC:
      return "panic";
    case ESP_RST_INT_WDT:
      return "int_watchdog";
    case ESP_RST_TASK_WDT:
      return "task_watchdog";
    case ESP_RST_WDT:
      return "watchdog";
    case ESP_RST_DEEPSLEEP:
      return "deepsleep";
    case ESP_RST_BROWNOUT:
      return "brownout";
    case ESP_RST_SDIO:
      return "sdio";
    default:
      return "unknown";
  }
}
} // namespace Board
