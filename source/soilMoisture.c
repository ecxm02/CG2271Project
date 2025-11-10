#include "includes/soilMoisture.h"
#include "board.h"
#include "includes/waterPump.h"

#define SOIL_PORT PORTA
#define SOIL_GPIO GPIOA
#define SOIL_PIN 1

static SemaphoreHandle_t gSoilSem = NULL;

void SoilMoisture_Init(void) {
    NVIC_DisableIRQ(PORTA_IRQn);
    
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    
    SOIL_PORT->PCR[SOIL_PIN] = (SOIL_PORT->PCR[SOIL_PIN] & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(1);
    SOIL_PORT->PCR[SOIL_PIN] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    
    SOIL_GPIO->PDDR &= ~(1 << SOIL_PIN);
    
    SOIL_PORT->PCR[SOIL_PIN] = (SOIL_PORT->PCR[SOIL_PIN] & ~PORT_PCR_IRQC_MASK) | PORT_PCR_IRQC(0b1011);
    
    NVIC_SetPriority(PORTA_IRQn, 192);
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    NVIC_EnableIRQ(PORTA_IRQn);
    
    gSoilSem = xSemaphoreCreateBinary();
}

SemaphoreHandle_t SoilMoisture_GetSemaphore(void) {
    return gSoilSem;
}

int SoilMoisture_Read(void) {
    return (SOIL_GPIO->PDIR & (1 << SOIL_PIN)) ? 1 : 0;
}

void PORTA_IRQHandler(void) {
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    
    if (PORTA->ISFR & (1 << SOIL_PIN)) {
        BaseType_t hpw = pdFALSE;
        if (gSoilSem) {
            xSemaphoreGiveFromISR(gSoilSem, &hpw);
        }
        PORTA->ISFR |= (1 << SOIL_PIN);
        portYIELD_FROM_ISR(hpw);
    }
}
