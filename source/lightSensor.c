#include "includes/lightSensor.h"
#include "board.h"
#include "includes/led.h"

#define LDR_PORT PORTC
#define LDR_GPIO GPIOC
#define LDR_PIN 2

void LightSensor_Init(void) {
    NVIC_DisableIRQ(PORTC_PORTD_IRQn);
    
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;
    
    LDR_PORT->PCR[LDR_PIN] &= ~PORT_PCR_MUX_MASK;
    LDR_PORT->PCR[LDR_PIN] |= PORT_PCR_MUX(1);
    
    LDR_PORT->PCR[LDR_PIN] &= ~PORT_PCR_PE_MASK;
    LDR_PORT->PCR[LDR_PIN] |= PORT_PCR_PE(1);
    LDR_PORT->PCR[LDR_PIN] &= ~PORT_PCR_PS_MASK;
    LDR_PORT->PCR[LDR_PIN] |= PORT_PCR_PS(1);
    
    LDR_GPIO->PDDR &= ~(1 << LDR_PIN);
    
    LDR_PORT->PCR[LDR_PIN] &= ~PORT_PCR_IRQC_MASK;
    LDR_PORT->PCR[LDR_PIN] |= PORT_PCR_IRQC(0b1011);
    
    NVIC_SetPriority(PORTC_PORTD_IRQn, 192);
    NVIC_ClearPendingIRQ(PORTC_PORTD_IRQn);
    NVIC_EnableIRQ(PORTC_PORTD_IRQn);
}

void PORTC_PORTD_IRQHandler(void) {
    NVIC_ClearPendingIRQ(PORTC_PORTD_IRQn);
    
    if(PORTC->ISFR & (1 << LDR_PIN)) {
        LED_Toggle(LED_BLUE);
    }
    
    PORTC->ISFR |= (1 << LDR_PIN);
}