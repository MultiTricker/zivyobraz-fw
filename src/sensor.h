#ifndef SENSOR_H
#define SENSOR_H

//////////////////////////////////////////////////////////////
// Uncomment if one of the sensors will be connected
// Supported sensors: SHT40/41/45, SCD40/41, BME280, STCC4
//////////////////////////////////////////////////////////////

// #define SENSOR

#include <Arduino.h>
#include <ArduinoJson.h>

#ifdef SENSOR

enum class SensorType : uint8_t
{
  NONE = 0,
  SHT4X = 1,
  BME280 = 2,
  SCD4X = 3,
  STCC4 = 4
};

// Sensor data structure for passing sensor readings
struct SensorData
{
  const char *type;
  float temperature;
  int humidity;
  int pressureOrCO2;
  bool isPressure;          // true = pressure (BME280), false = CO2 (SCD4X, STCC4)
  bool hasThirdMeasurement; // true if sensor provides pressure or CO2
  bool isValid;

  // Serialize sensor data to JSON object
  template <typename JsonArrayT>
  void toJson(JsonArrayT &sensors) const
  {
    if (!isValid || type == nullptr)
      return;

    auto sensor = sensors.template add<JsonObject>();
    sensor["type"] = type;
    sensor["temp"] = temperature;
    sensor["hum"] = humidity;

    // Only include pressure/CO2 if the sensor provides it
    if (hasThirdMeasurement)
    {
      if (isPressure)
        sensor["pres"] = pressureOrCO2;
      else
        sensor["co2"] = pressureOrCO2;
    }
  }
};

class Sensor
{
public:
  static Sensor &getInstance();

  void init();
  bool readSensorsVal(float &sen_temp, int &sen_humi, int &sen_pres);
  SensorType getSensorType() const;
  const char *getSensorTypeStr() const;
  SensorData getSensorData();

private:
  Sensor() = default;
  Sensor(const Sensor &) = delete;
  Sensor &operator=(const Sensor &) = delete;

  SensorType detectSensor();
  bool readSHT4X(float &sen_temp, int &sen_humi);
  bool readBME280(float &sen_temp, int &sen_humi, int &sen_pres);
  bool readSCD4X(float &sen_temp, int &sen_humi, int &sen_pres);
  bool readSTCC4(float &sen_temp, int &sen_humi, int &sen_pres);

  SensorType m_detectedSensor = SensorType::NONE;
};

#endif // SENSOR

#endif // SENSOR_H
