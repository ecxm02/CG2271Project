#ifndef WATER_PUMP_H
#define WATER_PUMP_H

#include <stdbool.h>

void WaterPump_Init(void);
void WaterPump_On(void);
void WaterPump_Off(void);
bool WaterPump_GetState(void);

#endif
