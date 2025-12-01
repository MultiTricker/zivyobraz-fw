#include "sensor.h"

#include "board.h"
#include "state_manager.h"

#ifdef SENSOR

  // SHT40/41/45
  #include "Adafruit_SHT4x.h"
static Adafruit_SHT4x sht4 = Adafruit_SHT4x();

  // SCD40/41
  #include "SparkFun_SCD4x_Arduino_Library.h"
static SCD4x SCD4(SCD4x_SENSOR_SCD41);

  // BME280
  #include <Adafruit_BME280.h>
  #include <Adafruit_Sensor.h>
static Adafruit_BME280 bme;

  // STCC4
  #include <SensirionI2cStcc4.h>
static SensirionI2cStcc4 stcc4;

namespace Sensor
{

// Store detected sensor type in RTC memory (persists through deep sleep)
RTC_DATA_ATTR SensorType detectedSensor = SensorType::NONE;

static SensorType detectSensor();
static bool readSHT4X(float &sen_temp, int &sen_humi);
static bool readBME280(float &sen_temp, int &sen_humi, int &sen_pres);
static bool readSCD4X(float &sen_temp, int &sen_humi, int &sen_pres);
static bool readSTCC4(float &sen_temp, int &sen_humi, int &sen_pres);

void init()
{
  StateManager::ResetReason reason = StateManager::getResetReason();

  // Reset sensor detection on power-on or hardware reset
  if (reason == StateManager::ResetReason::POWERON || reason == StateManager::ResetReason::EXT)
  {
    Serial.println("Fresh boot - resetting sensor detection");
    detectedSensor = SensorType::NONE;
  }
  else if (reason == StateManager::ResetReason::DEEPSLEEP)
  {
    Serial.print("Wake from deep sleep - using cached sensor: ");
    Serial.println(getSensorTypeStr());
  }

  // Detect sensor if not already known
  if (detectedSensor == SensorType::NONE)
  {
    detectedSensor = detectSensor();
    if (detectedSensor != SensorType::NONE)
    {
      Serial.print("Sensor detected and cached: ");
      Serial.println(getSensorTypeStr());
    }
    else
    {
      Serial.println("No sensor found");
    }
  }
}

static SensorType detectSensor()
{
  #if (defined ESPink_V2) || (defined ESPink_V3) || (defined ESPink_V35) || (defined ESP32S3Adapter)
  // LaskaKit ESPink 2.5 needs to power up uSup
  Board::setEPaperPowerOn(true);
  delay(50);
  #endif

  #if (defined PIN_SDA) && (defined PIN_SCL)
  Wire.begin(PIN_SDA, PIN_SCL);
  #endif

  SensorType found = SensorType::NONE;

  // Check for SHT40 OR SHT41 OR SHT45
  if (sht4.begin())
  {
    Serial.println("SHT4x FOUND");
    found = SensorType::SHT4X;
  }
  // Check for BME280
  else if (bme.begin())
  {
    Serial.println("BME280 FOUND");
    found = SensorType::BME280;
  }
  // Check for SCD40 OR SCD41
  else if (SCD4.begin(false, true, false))
  {
    Serial.println("SCD4x FOUND");
    found = SensorType::SCD4X;
  }
  // Check for STCC4
  else
  {
    stcc4.begin(Wire, STCC4_I2C_ADDR_64);
    uint32_t productId;
    uint64_t serialNumber;
    if (stcc4.getProductId(productId, serialNumber) == 0)
    {
      Serial.println("STCC4 FOUND");
      found = SensorType::STCC4;
    }
  }

  #if (defined ESPink_V2) || (defined ESPink_V3) || (defined ESPink_V35) || (defined ESP32S3Adapter)
  // Power down for now
  Board::setEPaperPowerOn(false);
  #endif

  return found;
}

bool readSensorsVal(float &sen_temp, int &sen_humi, int &sen_pres)
{
  if (detectedSensor == SensorType::NONE)
  {
    Serial.println("ERROR: No sensor detected");
    return false;
  }

  #if (defined ESPink_V2) || (defined ESPink_V3) || (defined ESPink_V35) || (defined ESP32S3Adapter)
  // LaskaKit ESPink 2.5 needs to power up uSup
  Board::setEPaperPowerOn(true);
  delay(50);
  #endif

  #if (defined PIN_SDA) && (defined PIN_SCL)
  Wire.begin(PIN_SDA, PIN_SCL);
  #endif

  bool ret = false;

  switch (detectedSensor)
  {
    case SensorType::SHT4X:
      ret = readSHT4X(sen_temp, sen_humi);
      break;

    case SensorType::BME280:
      ret = readBME280(sen_temp, sen_humi, sen_pres);
      break;

    case SensorType::SCD4X:
      ret = readSCD4X(sen_temp, sen_humi, sen_pres);
      break;

    case SensorType::STCC4:
      ret = readSTCC4(sen_temp, sen_humi, sen_pres);
      break;

    default:
      Serial.println("ERROR: Unknown sensor type");
      break;
  }

  if (!ret)
    Serial.println("ERROR: Failed to read sensor data");

  #if (defined ESPink_V2) || (defined ESPink_V3) || (defined ESPink_V35) || (defined ESP32S3Adapter)
  // Power down for now
  Board::setEPaperPowerOn(false);
  #endif

  return ret;
}

static bool readSHT4X(float &sen_temp, int &sen_humi)
{
  if (!sht4.begin())
  {
    Serial.println("ERROR: SHT4x not responding");
    return false;
  }

  sht4.setPrecision(SHT4X_LOW_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  sensors_event_t hum, temp;
  sht4.getEvent(&hum, &temp);

  sen_temp = temp.temperature;
  sen_humi = hum.relative_humidity;
  return true;
}

static bool readBME280(float &sen_temp, int &sen_humi, int &sen_pres)
{
  if (!bme.begin())
  {
    Serial.println("ERROR: BME280 not responding");
    return false;
  }

  sen_temp = bme.readTemperature();
  sen_humi = bme.readHumidity();
  sen_pres = bme.readPressure() / 100.0F;
  return true;
}

static bool readSCD4X(float &sen_temp, int &sen_humi, int &sen_pres)
{
  if (!SCD4.begin(false, true, false))
  {
    Serial.println("ERROR: SCD4x not responding");
    return false;
  }

  SCD4.measureSingleShot();

  while (SCD4.readMeasurement() == false) // wait for a new data (approx 30s)
  {
    Serial.println("Waiting for first measurement...");
    delay(1000);
  }

  sen_temp = SCD4.getTemperature();
  sen_humi = SCD4.getHumidity();
  sen_pres = SCD4.getCO2();
  return true;
}

static bool readSTCC4(float &sen_temp, int &sen_humi, int &sen_pres)
{
  stcc4.begin(Wire, STCC4_I2C_ADDR_64);

  uint32_t productId;
  uint64_t serialNumber;
  int16_t error = stcc4.getProductId(productId, serialNumber);

  if (error != 0)
  {
    Serial.println("ERROR: STCC4 not responding");
    return false;
  }

  stcc4.exitSleepMode(); // Ensure sensor is awake (optional safety measure)
  delay(50);

  // Warmup to get rid of the 390 ppm start value
  error = stcc4.startContinuousMeasurement();
  if (error)
  {
    Serial.println("ERROR: STCC4 failed to start continuous measurement");
    return false;
  }

  Serial.println("Waiting 30s to warmup STCC4");
  delay(30 * 1000);
  Serial.println("STCC4 warmup complete");

  error = stcc4.stopContinuousMeasurement();
  if (error)
  {
    Serial.println("ERROR: STCC4 failed to stop continuous measurement");
    return false;
  }

  // Perform single shot measurement
  error = stcc4.measureSingleShot();
  if (error != 0)
  {
    Serial.println("ERROR: STCC4 single shot measurement failed");
    return false;
  }

  // Wait for measurement to complete (typical ~5 seconds for STCC4)
  delay(5000);

  // Temporary variables to hold raw sensor data
  int16_t temp_cpo2;
  float temp_temp;
  float temp_humidity;
  uint16_t sensorStatus;

  // Read measurement
  error = stcc4.readMeasurement(temp_cpo2, temp_temp, temp_humidity, sensorStatus);
  if (error != 0)
  {
    Serial.println("ERROR: STCC4 read measurement failed");
    return false;
  }

  // Assign to output variables
  sen_temp = temp_temp;
  sen_humi = (int)temp_humidity;
  sen_pres = temp_cpo2;
  return true;
}

SensorType getSensorType() { return detectedSensor; }

const char *getSensorTypeStr()
{
  switch (detectedSensor)
  {
    case SensorType::NONE:
      return "NONE";
    case SensorType::SHT4X:
      return "SHT4X";
    case SensorType::BME280:
      return "BME280";
    case SensorType::SCD4X:
      return "SCD4X";
    case SensorType::STCC4:
      return "STCC4";
    default:
      return "UNKNOWN";
  }
}

} // namespace Sensor

#endif
