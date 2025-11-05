#ifndef WATER_LEVEL_H
#define WATER_LEVEL_H

#include <stdint.h>

#define WATER_LOW_THRESHOLD 20

void WaterLevel_Init(void);
uint16_t WaterLevel_ReadRaw(void);
uint8_t WaterLevel_GetPercentage(void);
uint8_t WaterLevel_IsLow(void);

#endif
