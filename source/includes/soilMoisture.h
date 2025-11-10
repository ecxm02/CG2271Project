#ifndef SOIL_MOISTURE_H
#define SOIL_MOISTURE_H

#include "FreeRTOS.h"
#include "semphr.h"

void SoilMoisture_Init(void);
SemaphoreHandle_t SoilMoisture_GetSemaphore(void);
int SoilMoisture_Read(void);

#endif
