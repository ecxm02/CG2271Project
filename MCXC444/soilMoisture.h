#ifndef SOIL_MOISTURE_H
#define SOIL_MOISTURE_H

#include <stdint.h>

void SoilMoisture_Init(void);
uint8_t SoilMoisture_Read(void);
uint8_t SoilMoisture_IsDry(void);

#endif
