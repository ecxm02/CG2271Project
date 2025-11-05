#include "soilMoisture.h"
#include "board.h"

#define SOIL_PORT PORTA
#define SOIL_GPIO GPIOA
#define SOIL_PIN 1

static volatile uint8_t soilState = 0;

void SoilMoisture_Init(void) {
    NVIC_DisableIRQ(PORTA_IRQn);
    
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    
    SOIL_PORT->PCR[SOIL_PIN] &= ~PORT_PCR_MUX_MASK;
    SOIL_PORT->PCR[SOIL_PIN] |= PORT_PCR_MUX(1);
    
    SOIL_PORT->PCR[SOIL_PIN] |= PORT_PCR_PE(1) | PORT_PCR_PS(1);
    
    SOIL_GPIO->PDDR &= ~(1 << SOIL_PIN);
    
    SOIL_PORT->PCR[SOIL_PIN] &= ~PORT_PCR_IRQC_MASK;
    SOIL_PORT->PCR[SOIL_PIN] |= PORT_PCR_IRQC(0b1011);
    
    NVIC_SetPriority(PORTA_IRQn, 255);
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    NVIC_EnableIRQ(PORTA_IRQn);
    
    soilState = (SOIL_GPIO->PDIR & (1 << SOIL_PIN)) ? 1 : 0;
}

uint8_t SoilMoisture_Read(void) {
    return soilState;
}

uint8_t SoilMoisture_IsDry(void) {
    return soilState;
}

void PORTA_IRQHandler(void) {
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    
    if(PORTA->ISFR & (1 << SOIL_PIN)) {
        // Read the new state and clear the interrupt flag
        soilState = (SOIL_GPIO->PDIR & (1 << SOIL_PIN)) ? 1 : 0;
        PORTA->ISFR |= (1 << SOIL_PIN);
    }
}