#include "lightSensor.h"
#include "board.h"

#define LDR_PORT PORTE
#define LDR_GPIO GPIOE
#define LDR_PIN 30

void LightSensor_Init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
    
    LDR_PORT->PCR[LDR_PIN] = PORT_PCR_MUX(1);
    
    LDR_GPIO->PDDR &= ~(1 << LDR_PIN);
}

uint8_t LightSensor_Read(void) {
    return (LDR_GPIO->PDIR & (1 << LDR_PIN)) ? 1 : 0;
}

uint8_t LightSensor_IsDark(void) {
    return LightSensor_Read();
}