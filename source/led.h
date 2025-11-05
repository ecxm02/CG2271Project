#ifndef LED_H
#define LED_H

#include <stdint.h>

typedef enum {
    LED_RED = 0,
    LED_GREEN = 1,
    LED_BLUE = 2
} LED_Color_t;

void LED_Init(void);
void LED_SetColor(LED_Color_t color, uint8_t state);
void LED_On(LED_Color_t color);
void LED_Off(LED_Color_t color);
void LED_Toggle(LED_Color_t color);
void LED_AllOff(void);
uint8_t LED_GetStatus(LED_Color_t color);

#endif
