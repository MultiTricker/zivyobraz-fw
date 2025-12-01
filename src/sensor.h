#ifndef SENSOR_H
#define SENSOR_H

//////////////////////////////////////////////////////////////
// Uncomment if one of the sensors will be connected
// Supported sensors: SHT40/41/45, SCD40/41, BME280, STCC4
//////////////////////////////////////////////////////////////

// #define SENSOR

#include <Arduino.h>

namespace Sensor
{

enum class SensorType : uint8_t
{
  NONE = 0,
  SHT4X = 1,
  BME280 = 2,
  SCD4X = 3,
  STCC4 = 4
};

void init();
bool readSensorsVal(float &sen_temp, int &sen_humi, int &sen_pres);

SensorType getSensorType();
const char *getSensorTypeStr();

} // namespace Sensor

#endif // SENSOR_H
