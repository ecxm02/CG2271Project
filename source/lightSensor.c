#include "lightSensor.h"
#include "board.h"
#include "led.h"

#define LDR_PORT PORTC
#define LDR_GPIO GPIOC
#define LDR_PIN 2

static volatile uint8_t lightState = 0;

void LightSensor_Init(void) {
    NVIC_DisableIRQ(PORTC_PORTD_IRQn);
    
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;
    
    LDR_PORT->PCR[LDR_PIN] &= ~PORT_PCR_MUX_MASK;
    LDR_PORT->PCR[LDR_PIN] |= PORT_PCR_MUX(1);
    
    LDR_PORT->PCR[LDR_PIN] |= PORT_PCR_PE(1) | PORT_PCR_PS(1);
    
    LDR_GPIO->PDDR &= ~(1 << LDR_PIN);
    
    LDR_PORT->PCR[LDR_PIN] &= ~PORT_PCR_IRQC_MASK;
    LDR_PORT->PCR[LDR_PIN] |= PORT_PCR_IRQC(0b1011);
    
    NVIC_SetPriority(PORTC_PORTD_IRQn, 255);
    NVIC_ClearPendingIRQ(PORTC_PORTD_IRQn);
    NVIC_EnableIRQ(PORTC_PORTD_IRQn);
    
    lightState = (LDR_GPIO->PDIR & (1 << LDR_PIN)) ? 1 : 0;
}

uint8_t LightSensor_Read(void) {
    return lightState;
}

uint8_t LightSensor_IsDark(void) {
    return lightState;
}

void PORTC_PORTD_IRQHandler(void) {
    NVIC_ClearPendingIRQ(PORTC_PORTD_IRQn);
    
    if(PORTC->ISFR & (1 << LDR_PIN)) {
        lightState = (GPIOC->PDIR & (1 << LDR_PIN)) ? 1 : 0;
        
        if(lightState) {
            GPIOA->PSOR = (1 << 4) | (1 << 5);
            GPIOD->PSOR = (1 << 4);
        } else {
            GPIOA->PCOR = (1 << 4) | (1 << 5);
            GPIOD->PCOR = (1 << 4);
        }
        
        PORTC->ISFR |= (1 << LDR_PIN);
    }
}