#include "sensor.h"

#include "board.h"

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

int readSensorsVal(float &sen_temp, int &sen_humi, int &sen_pres)
{
  int ret = 0;

  #if (defined ESPink_V2) || (defined ESPink_V3) || (defined ESPink_V35) || (defined ESP32S3Adapter)
  // LaskaKit ESPink 2.5 needs to power up uSup
  Board::setEPaperPowerOn(true);
  delay(50);
  #endif

  #if (defined PIN_SDA) && (defined PIN_SCL)
  Wire.begin(PIN_SDA, PIN_SCL);
  #endif

  // Check for SHT40 OR SHT41 OR SHT45
  if (sht4.begin())
  {
    Serial.println("SHT4x FOUND");
    sht4.setPrecision(SHT4X_LOW_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);

    sensors_event_t hum, temp;
    sht4.getEvent(&hum, &temp);

    sen_temp = temp.temperature;
    sen_humi = hum.relative_humidity;
    ret = 1;
  }

  // Check for BME280
  if (!ret && bme.begin())
  {
    Serial.println("BME280 FOUND");

    sen_temp = bme.readTemperature();
    sen_humi = bme.readHumidity();
    sen_pres = bme.readPressure() / 100.0F;
    ret = 2;
  }

  // Check for SCD40 OR SCD41
  if (!ret && SCD4.begin(false, true, false))
  {
    Serial.println("SCD4x FOUND");
    SCD4.measureSingleShot();

    while (SCD4.readMeasurement() == false) // wait for a new data (approx 30s)
    {
      Serial.println("Waiting for first measurement...");
      delay(1000);
    }

    sen_temp = SCD4.getTemperature();
    sen_humi = SCD4.getHumidity();
    sen_pres = SCD4.getCO2();
    ret = 3;
  }

  // Check for STCC4
  if (!ret)
  {
    stcc4.begin(Wire, STCC4_I2C_ADDR_64);
    
    uint32_t productId;
    uint64_t serialNumber;
    int16_t error = stcc4.getProductId(productId, serialNumber);
    
    if (error == 0)
    {
      Serial.println("STCC4 FOUND");
      
      stcc4.exitSleepMode();  // Ensure sensor is awake (optional safety measure)
      delay(50);

      // Warmup to get rid of the 390 ppm start value
      error = stcc4.startContinuousMeasurement();
      if (error) { return false; }
      Serial.println("Waiting 30s to warmup STCC4");
      delay(30 * 1000);
      Serial.println("STCC4 warmup complete");

      error = stcc4.stopContinuousMeasurement();
      if (error) { return false; }

      // Perform single shot measurement
      error = stcc4.measureSingleShot();
      if (error == 0)
      {
        // Wait for measurement to complete (typical ~5 seconds for STCC4)
        delay(5000);
        
        // Temporary variables to hold raw sensor data
        int16_t temp_cpo2;
        float temp_temp;
        float temp_humidity;
        uint16_t sensorStatus;
        
        // Read measurement
        error = stcc4.readMeasurement(temp_cpo2, temp_temp, temp_humidity, sensorStatus);
        if (error == 0)
        {
          // Assign to output variables
          sen_temp = temp_temp;
          sen_humi = (int)temp_humidity;
          sen_pres = temp_cpo2;
          ret = 3; // Same return code as SCD4x
        }
        else
        {
          Serial.println("STCC4 read measurement failed");
        }
      }
      else
      {
        Serial.println("STCC4 single shot measurement failed");
      }
    }
  }

  #if (defined ESPink_V2) || (defined ESPink_V3) || (defined ESPink_V35) || (defined ESP32S3Adapter)
  // Power down for now
  Board::setEPaperPowerOn(false);
  #endif
  return ret;
}
} // namespace Sensor

#endif
