#include "buzzer.h"
#include "board.h"

#define BUZZER_PORT PORTD
#define BUZZER_GPIO GPIOD
#define BUZZER_PIN 3
#define BUZZER_PCR_MUX 1

static uint8_t buzzerStatus = 0;

void Buzzer_Init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
    BUZZER_PORT->PCR[BUZZER_PIN] = PORT_PCR_MUX(BUZZER_PCR_MUX);
    BUZZER_GPIO->PDDR |= (1 << BUZZER_PIN);
    Buzzer_Off();
}

void Buzzer_On(void) {
    BUZZER_GPIO->PSOR = (1 << BUZZER_PIN);
    buzzerStatus = 1;
}

void Buzzer_Off(void) {
    BUZZER_GPIO->PCOR = (1 << BUZZER_PIN);
    buzzerStatus = 0;
}

void Buzzer_Toggle(void) {
    BUZZER_GPIO->PTOR = (1 << BUZZER_PIN);
    buzzerStatus = !buzzerStatus;
}

void Buzzer_Beep(uint16_t duration_ms) {
    Buzzer_On();
    for (volatile uint32_t i = 0; i < (duration_ms * 1000); i++);
    Buzzer_Off();
}

uint8_t Buzzer_GetStatus(void) {
    return buzzerStatus;
}
