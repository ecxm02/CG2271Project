#include "includes/waterPump.h"
#include "board.h"

#define PUMP_PORT PORTA
#define PUMP_GPIO GPIOA
#define PUMP_PIN 2

void WaterPump_Init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    
    PUMP_PORT->PCR[PUMP_PIN] &= ~PORT_PCR_MUX_MASK;
    PUMP_PORT->PCR[PUMP_PIN] |= PORT_PCR_MUX(1);
    
    PUMP_GPIO->PDDR |= (1 << PUMP_PIN);
    PUMP_GPIO->PCOR = (1 << PUMP_PIN);
}

void WaterPump_On(void) {
    PUMP_GPIO->PSOR = (1 << PUMP_PIN);
}

void WaterPump_Off(void) {
    PUMP_GPIO->PCOR = (1 << PUMP_PIN);
}
