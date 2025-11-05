#include "includes/soilMoisture.h"
#include "board.h"
#include "includes/waterPump.h"

#define SOIL_PORT PORTA
#define SOIL_GPIO GPIOA
#define SOIL_PIN 1

static volatile bool relayState = false;
static volatile int lastSoilState = 0;

void SoilMoisture_Init(void) {
    NVIC_DisableIRQ(PORTA_IRQn);
    
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    
    SOIL_PORT->PCR[SOIL_PIN] &= ~PORT_PCR_MUX_MASK;
    SOIL_PORT->PCR[SOIL_PIN] |= PORT_PCR_MUX(1);
    
    SOIL_PORT->PCR[SOIL_PIN] &= ~PORT_PCR_PE_MASK;
    SOIL_PORT->PCR[SOIL_PIN] |= PORT_PCR_PE(1);
    SOIL_PORT->PCR[SOIL_PIN] &= ~PORT_PCR_PS_MASK;
    SOIL_PORT->PCR[SOIL_PIN] |= PORT_PCR_PS(1);
    
    SOIL_GPIO->PDDR &= ~(1 << SOIL_PIN);
    
    SOIL_PORT->PCR[SOIL_PIN] &= ~PORT_PCR_IRQC_MASK;
    SOIL_PORT->PCR[SOIL_PIN] |= PORT_PCR_IRQC(0b1011);
    
    NVIC_SetPriority(PORTA_IRQn, 192);
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    NVIC_EnableIRQ(PORTA_IRQn);
}

void PORTA_IRQHandler(void) {
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    
    if(PORTA->ISFR & (1 << SOIL_PIN)) {
        int currentSoilState = GPIOA->PDIR & (1 << SOIL_PIN);
        
        if (currentSoilState != lastSoilState) {
            relayState = !relayState;
            
            if (relayState) {
                WaterPump_On();
            } else {
                WaterPump_Off();
            }
            
            lastSoilState = currentSoilState;
        }
    }
    
    PORTA->ISFR |= (1 << SOIL_PIN);
}
