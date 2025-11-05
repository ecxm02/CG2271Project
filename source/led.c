#include "includes/led.h"
#include "board.h"

#define LED_BLUE_PORT PORTA
#define LED_BLUE_GPIO GPIOA
#define LED_BLUE_PIN 4

void LED_Init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    
    LED_BLUE_PORT->PCR[LED_BLUE_PIN] &= ~PORT_PCR_MUX_MASK;
    LED_BLUE_PORT->PCR[LED_BLUE_PIN] |= PORT_PCR_MUX(1);
    
    LED_BLUE_GPIO->PDDR |= (1 << LED_BLUE_PIN);
    LED_BLUE_GPIO->PCOR = (1 << LED_BLUE_PIN);
}

void LED_On(uint8_t led) {
    if(led == LED_BLUE) {
        LED_BLUE_GPIO->PSOR = (1 << LED_BLUE_PIN);
    }
}

void LED_Off(uint8_t led) {
    if(led == LED_BLUE) {
        LED_BLUE_GPIO->PCOR = (1 << LED_BLUE_PIN);
    }
}

void LED_Toggle(uint8_t led) {
    if(led == LED_BLUE) {
        LED_BLUE_GPIO->PTOR = (1 << LED_BLUE_PIN);
    }
}
