#include "includes/lightSensor.h"
#include "board.h"

#define LDR_PORT PORTC
#define LDR_GPIO GPIOC
#define LDR_PIN 2

void LightSensor_Init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;
    
    // Configure as GPIO
    LDR_PORT->PCR[LDR_PIN] &= ~PORT_PCR_MUX_MASK;
    LDR_PORT->PCR[LDR_PIN] |= PORT_PCR_MUX(1);
    
    // Enable pull-up resistor
    LDR_PORT->PCR[LDR_PIN] &= ~PORT_PCR_PE_MASK;
    LDR_PORT->PCR[LDR_PIN] |= PORT_PCR_PE(1);
    LDR_PORT->PCR[LDR_PIN] &= ~PORT_PCR_PS_MASK;
    LDR_PORT->PCR[LDR_PIN] |= PORT_PCR_PS(1);
    
    // Configure as input
    LDR_GPIO->PDDR &= ~(1 << LDR_PIN);
}

uint8_t LightSensor_Read(void) {
    // Read the LDR sensor state (GPIO input read)
    // Returns 1 if light detected, 0 if dark
    return (LDR_GPIO->PDIR & (1 << LDR_PIN)) ? 1 : 0;
}