#include "waterPump.h"
#include "board.h"

#define PUMP_PORT PORTA
#define PUMP_GPIO GPIOA
#define PUMP_PIN 2
#define PUMP_PCR_MUX 1

static uint8_t pumpStatus = 0;

void WaterPump_Init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    PUMP_PORT->PCR[PUMP_PIN] = PORT_PCR_MUX(PUMP_PCR_MUX);
    PUMP_GPIO->PDDR |= (1 << PUMP_PIN);
    WaterPump_Off();
}

void WaterPump_On(void) {
    PUMP_GPIO->PSOR = (1 << PUMP_PIN);
    pumpStatus = 1;
}

void WaterPump_Off(void) {
    PUMP_GPIO->PCOR = (1 << PUMP_PIN);
    pumpStatus = 0;
}

void WaterPump_Toggle(void) {
    PUMP_GPIO->PTOR = (1 << PUMP_PIN);
    pumpStatus = !pumpStatus;
}

uint8_t WaterPump_GetStatus(void) {
    return pumpStatus;
}