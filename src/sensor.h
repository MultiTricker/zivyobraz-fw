#ifndef SENSOR_H
#define SENSOR_H

//////////////////////////////////////////////////////////////
// Uncomment if one of the sensors will be connected
// Supported sensors: SHT40/41/45, SCD40/41, BME280, STCC4
//////////////////////////////////////////////////////////////

// #define SENSOR

namespace Sensor
{
int readSensorsVal(float &sen_temp, int &sen_humi, int &sen_pres);
} // namespace Sensor

#endif // SENSOR_H
