#include "soilMoisture.h"
#include "board.h"

#define SOIL_PORT PORTB
#define SOIL_GPIO GPIOB
#define SOIL_PIN 0

void SoilMoisture_Init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
    
    SOIL_PORT->PCR[SOIL_PIN] = PORT_PCR_MUX(1);
    
    SOIL_GPIO->PDDR &= ~(1 << SOIL_PIN);
}

uint8_t SoilMoisture_Read(void) {
    return (SOIL_GPIO->PDIR & (1 << SOIL_PIN)) ? 1 : 0;
}

uint8_t SoilMoisture_IsDry(void) {
    return SoilMoisture_Read();
}