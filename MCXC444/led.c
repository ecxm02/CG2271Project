#include "led.h"
#include "MKL25Z4.h"

#define LED_RED_PORT PORTA
#define LED_RED_GPIO GPIOA
#define LED_RED_PIN 4

#define LED_GREEN_PORT PORTD
#define LED_GREEN_GPIO GPIOD
#define LED_GREEN_PIN 4

#define LED_BLUE_PORT PORTA
#define LED_BLUE_GPIO GPIOA
#define LED_BLUE_PIN 5

static uint8_t ledStatus[3] = {0, 0, 0};

void LED_Init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTD_MASK;
    
    LED_RED_PORT->PCR[LED_RED_PIN] = PORT_PCR_MUX(1);
    LED_RED_GPIO->PDDR |= (1 << LED_RED_PIN);
    
    LED_GREEN_PORT->PCR[LED_GREEN_PIN] = PORT_PCR_MUX(1);
    LED_GREEN_GPIO->PDDR |= (1 << LED_GREEN_PIN);
    
    LED_BLUE_PORT->PCR[LED_BLUE_PIN] = PORT_PCR_MUX(1);
    LED_BLUE_GPIO->PDDR |= (1 << LED_BLUE_PIN);
    
    LED_AllOff();
}

void LED_SetColor(LED_Color_t color, uint8_t state) {
    if (state) {
        LED_On(color);
    } else {
        LED_Off(color);
    }
}

void LED_On(LED_Color_t color) {
    switch(color) {
        case LED_RED:
            LED_RED_GPIO->PSOR = (1 << LED_RED_PIN);
            ledStatus[LED_RED] = 1;
            break;
        case LED_GREEN:
            LED_GREEN_GPIO->PSOR = (1 << LED_GREEN_PIN);
            ledStatus[LED_GREEN] = 1;
            break;
        case LED_BLUE:
            LED_BLUE_GPIO->PSOR = (1 << LED_BLUE_PIN);
            ledStatus[LED_BLUE] = 1;
            break;
    }
}

void LED_Off(LED_Color_t color) {
    switch(color) {
        case LED_RED:
            LED_RED_GPIO->PCOR = (1 << LED_RED_PIN);
            ledStatus[LED_RED] = 0;
            break;
        case LED_GREEN:
            LED_GREEN_GPIO->PCOR = (1 << LED_GREEN_PIN);
            ledStatus[LED_GREEN] = 0;
            break;
        case LED_BLUE:
            LED_BLUE_GPIO->PCOR = (1 << LED_BLUE_PIN);
            ledStatus[LED_BLUE] = 0;
            break;
    }
}

void LED_Toggle(LED_Color_t color) {
    switch(color) {
        case LED_RED:
            LED_RED_GPIO->PTOR = (1 << LED_RED_PIN);
            ledStatus[LED_RED] = !ledStatus[LED_RED];
            break;
        case LED_GREEN:
            LED_GREEN_GPIO->PTOR = (1 << LED_GREEN_PIN);
            ledStatus[LED_GREEN] = !ledStatus[LED_GREEN];
            break;
        case LED_BLUE:
            LED_BLUE_GPIO->PTOR = (1 << LED_BLUE_PIN);
            ledStatus[LED_BLUE] = !ledStatus[LED_BLUE];
            break;
    }
}

void LED_AllOff(void) {
    LED_Off(LED_RED);
    LED_Off(LED_GREEN);
    LED_Off(LED_BLUE);
}

uint8_t LED_GetStatus(LED_Color_t color) {
    return ledStatus[color];
}
