#ifndef WATER_PUMP_H
#define WATER_PUMP_H

#include <stdint.h>

void WaterPump_Init(void);
void WaterPump_On(void);
void WaterPump_Off(void);
void WaterPump_Toggle(void);
uint8_t WaterPump_GetStatus(void);

#endif
