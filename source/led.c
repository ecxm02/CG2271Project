#include "includes/led.h"
#include "board.h"

#define LED_RED_PORT PORTA
#define LED_RED_GPIO GPIOA
#define LED_RED_PIN 4

#define LED_GREEN_PORT PORTD
#define LED_GREEN_GPIO GPIOD
#define LED_GREEN_PIN 4

#define LED_BLUE_PORT PORTA
#define LED_BLUE_GPIO GPIOA
#define LED_BLUE_PIN 5

void LED_Init(void) {
    SIM->SCGC5 |= (SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTD_MASK);
    
    // Red LED
    LED_RED_PORT->PCR[LED_RED_PIN] &= ~PORT_PCR_MUX_MASK;
    LED_RED_PORT->PCR[LED_RED_PIN] |= PORT_PCR_MUX(1);
    LED_RED_GPIO->PDDR |= (1 << LED_RED_PIN);
    LED_RED_GPIO->PCOR = (1 << LED_RED_PIN);
    
    // Green LED
    LED_GREEN_PORT->PCR[LED_GREEN_PIN] &= ~PORT_PCR_MUX_MASK;
    LED_GREEN_PORT->PCR[LED_GREEN_PIN] |= PORT_PCR_MUX(1);
    LED_GREEN_GPIO->PDDR |= (1 << LED_GREEN_PIN);
    LED_GREEN_GPIO->PCOR = (1 << LED_GREEN_PIN);
    
    // Blue LED
    LED_BLUE_PORT->PCR[LED_BLUE_PIN] &= ~PORT_PCR_MUX_MASK;
    LED_BLUE_PORT->PCR[LED_BLUE_PIN] |= PORT_PCR_MUX(1);
    LED_BLUE_GPIO->PDDR |= (1 << LED_BLUE_PIN);
    LED_BLUE_GPIO->PCOR = (1 << LED_BLUE_PIN);
}

void LED_On(uint8_t led) {
    if(led == LED_RED) {
        LED_RED_GPIO->PSOR = (1 << LED_RED_PIN);
    } else if(led == LED_GREEN) {
        LED_GREEN_GPIO->PSOR = (1 << LED_GREEN_PIN);
    } else if(led == LED_BLUE) {
        LED_BLUE_GPIO->PSOR = (1 << LED_BLUE_PIN);
    }
}

void LED_Off(uint8_t led) {
    if(led == LED_RED) {
        LED_RED_GPIO->PCOR = (1 << LED_RED_PIN);
    } else if(led == LED_GREEN) {
        LED_GREEN_GPIO->PCOR = (1 << LED_GREEN_PIN);
    } else if(led == LED_BLUE) {
        LED_BLUE_GPIO->PCOR = (1 << LED_BLUE_PIN);
    }
}

void LED_Toggle(uint8_t led) {
    if(led == LED_RED) {
        LED_RED_GPIO->PTOR = (1 << LED_RED_PIN);
    } else if(led == LED_GREEN) {
        LED_GREEN_GPIO->PTOR = (1 << LED_GREEN_PIN);
    } else if(led == LED_BLUE) {
        LED_BLUE_GPIO->PTOR = (1 << LED_BLUE_PIN);
    }
}

void LED_AllOn(void) {
    LED_RED_GPIO->PSOR = (1 << LED_RED_PIN);
    LED_GREEN_GPIO->PSOR = (1 << LED_GREEN_PIN);
    LED_BLUE_GPIO->PSOR = (1 << LED_BLUE_PIN);
}

void LED_AllOff(void) {
    LED_RED_GPIO->PCOR = (1 << LED_RED_PIN);
    LED_GREEN_GPIO->PCOR = (1 << LED_GREEN_PIN);
    LED_BLUE_GPIO->PCOR = (1 << LED_BLUE_PIN);
}
