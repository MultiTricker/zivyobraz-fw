#include "sensor.h"

#include "board.h"
#include "logger.h"
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

// Store detected sensor type in RTC memory (persists through deep sleep)
RTC_DATA_ATTR SensorType detectedSensor = SensorType::NONE;

Sensor &Sensor::getInstance()
{
  static Sensor instance;
  return instance;
}

void Sensor::init()
{
  ResetReason reason = Board::getResetReason();

  // Reset sensor detection on power-on or hardware reset
  if (reason == ResetReason::POWERON || reason == ResetReason::EXT)
  {
    Logger::log(Logger::Topic::SENS, "Fresh boot - resetting detection\n");
    detectedSensor = SensorType::NONE;
    m_detectedSensor = SensorType::NONE;
  }
  else if (reason == ResetReason::DEEPSLEEP)
  {
    m_detectedSensor = detectedSensor;
    Logger::log(Logger::Topic::SENS, "Wake from deep sleep - using cached sensor: {}\n", getSensorTypeStr());
  }

  // Detect sensor if not already known
  if (m_detectedSensor == SensorType::NONE)
  {
    m_detectedSensor = detectSensor();
    detectedSensor = m_detectedSensor;

    if (m_detectedSensor != SensorType::NONE)
      LoLogger::log(Logger::Topic::SENS, "Detected and cached: {}\n", getSensorTypeStr());
    else
      LoLogger::log(Logger::Topic::SENS, "No sensor found\n");
  }
}

SensorType Sensor::detectSensor()
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
    LoLogger::log(Logger::Topic::SENS, "SHT4x FOUND\n");
    found = SensorType::SHT4X;
  }
  // Check for BME280
  else if (bme.begin())
  {
    LoLogger::log(Logger::Topic::SENS, "BME280 FOUND\n");
    found = SensorType::BME280;
  }
  // Check for SCD40 OR SCD41
  else if (SCD4.begin(false, true, false))
  {
    LoLogger::log(Logger::Topic::SENS, "SCD4x FOUND\n");
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
      LogLogger::log(Logger::Topic::SENS, "STCC4 FOUND\n");
      found = SensorType::STCC4;
    }
  }

  #if (defined ESPink_V2) || (defined ESPink_V3) || (defined ESPink_V35) || (defined ESP32S3Adapter)
  // Power down for now
  Board::setEPaperPowerOn(false);
  #endif

  return found;
}

bool Sensor::readSensorsVal(float &sen_temp, int &sen_humi, int &sen_pres)
{
  if (m_detectedSensor == SensorType::NONE)
  {
    LoLogger::log(Logger::Topic::SENS, "No sensor detected\n");
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

  switch (m_detectedSensor)
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
      LoLogger::log(Logger::Topic::SENS, "ERROR: Unknown sensor type\n");
      break;
  }

  if (!ret)
    LoLogger::log(Logger::Topic::SENS, "Failed to read sensor data\n");

  #if (defined ESPink_V2) || (defined ESPink_V3) || (defined ESPink_V35) || (defined ESP32S3Adapter)
  // Power down for now
  Board::setEPaperPowerOn(false);
  #endif

  return ret;
}

bool Sensor::readSHT4X(float &sen_temp, int &sen_humi)
{
  if (!sht4.begin())
  {
    LLogger::log(Logger::Topic::SENS, "ERROR: SHT4x not responding\n");
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

bool Sensor::readBME280(float &sen_temp, int &sen_humi, int &sen_pres)
{
  if (!bme.begin())
  {
    LLogger::log(Logger::Topic::SENS, "ERROR: BME280 not responding\n");
    return false;
  }

  sen_temp = bme.readTemperature();
  sen_humi = bme.readHumidity();
  sen_pres = bme.readPressure() / 100.0F;
  return true;
}

bool Sensor::readSCD4X(float &sen_temp, int &sen_humi, int &sen_pres)
{
  if (!SCD4.begin(false, true, false))
  {
    LLogger::log(Logger::Topic::SENS, "ERROR: SCD4x not responding\n");
    return false;
  }

  SCD4.measureSingleShot();

  while (SCD4.readMeasurement() == false) // wait for a new data (approx 30s)
  {
    LoLogger::log(Logger::Topic::SENS, "Waiting for first measurement...\n");
    delay(1000);
  }

  sen_temp = SCD4.getTemperature();
  sen_humi = SCD4.getHumidity();
  sen_pres = SCD4.getCO2();
  return true;
}

bool Sensor::readSTCC4(float &sen_temp, int &sen_humi, int &sen_pres)
{
  stcc4.begin(Wire, STCC4_I2C_ADDR_64);

  uint32_t productId;
  uint64_t serialNumber;
  int16_t error = stcc4.getProductId(productId, serialNumber);

  if (error != 0)
  {
    LLogger::log(Logger::Topic::SENS, "ERROR: STCC4 not responding\n");
    return false;
  }

  stcc4.exitSleepMode(); // Ensure sensor is awake (optional safety measure)
  delay(50);

  // Warmup to get rid of the 390 ppm start value
  error = stcc4.startContinuousMeasurement();
  if (error)
  {
    LoLogger::log(Logger::Topic::SENS, "ERROR: STCC4 failed to start continuous measurement\n");
    return false;
  }

  LoLogger::log(Logger::Topic::SENS, "Waiting 30s to warmup STCC4\n");
  delay(30 * 1000);
  LoLogger::log(Logger::Topic::SENS, "STCC4 warmup complete\n");

  error = stcc4.stopContinuousMeasurement();
  if (error)
  {
    LoLogger::log(Logger::Topic::SENS, "ERROR: STCC4 failed to stop continuous measurement\n");
    return false;
  }

  // Perform single shot measurement
  error = stcc4.measureSingleShot();
  if (error != 0)
  {
    LoLogger::log(Logger::Topic::SENS, "ERROR: STCC4 single shot measurement failed\n");
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
    LoLogger::log(Logger::Topic::SENS, "ERROR: STCC4 readMeasurement error code: {}\n", error);
    return false;
  }

  // Assign to output variables
  sen_temp = temp_temp;
  sen_humi = (int)temp_humidity;
  sen_pres = temp_cpo2;
  return true;
}

SensorType Sensor::getSensorType() const { return m_detectedSensor; }

const char *Sensor::getSensorTypeStr() const
{
  switch (m_detectedSensor)
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

SensorData Sensor::getSensorData()
{
  SensorData data = {nullptr, 0.0f, 0, 0, false, false, false};

  float temperature;
  int humidity;
  int pressure = 0;

  if (readSensorsVal(temperature, humidity, pressure))
  {
    data.type = getSensorTypeStr();
    data.temperature = temperature;
    data.humidity = humidity;
    data.pressureOrCO2 = pressure;
    data.isPressure = (m_detectedSensor == SensorType::BME280);
    // Only BME280, SCD4X, and STCC4 have a third measurement
    data.hasThirdMeasurement = (m_detectedSensor == SensorType::BME280 || m_detectedSensor == SensorType::SCD4X ||
                                m_detectedSensor == SensorType::STCC4);
    data.isValid = true;
  }

  return data;
}

#endif // SENSOR
