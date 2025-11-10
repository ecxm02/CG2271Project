#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include <stdint.h>

void LightSensor_Init(void);
uint8_t LightSensor_Read(void);

#endif
