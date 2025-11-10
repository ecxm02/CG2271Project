#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>

#define LED_RED   0
#define LED_GREEN 1
#define LED_BLUE  2

void LED_Init(void);
void LED_On(uint8_t led);
void LED_Off(uint8_t led);
void LED_Toggle(uint8_t led);
void LED_AllOn(void);
void LED_AllOff(void);
bool LED_GetState(void);

#endif
