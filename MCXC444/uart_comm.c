#include "uart_comm.h"
#include "MKL25Z4.h"
#include <stdio.h>
#include <string.h>

#define UART_OVERSAMPLE_RATE 16
#define SYSTEM_CLOCK 48000000

static volatile uint8_t rxBuffer[64];
static volatile uint8_t rxIndex = 0;
static volatile uint8_t commandReady = 0;

void UART_Init(void) {
    uint16_t sbr;
    
    SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    
    PORTA->PCR[1] = PORT_PCR_MUX(2);
    PORTA->PCR[2] = PORT_PCR_MUX(2);
    
    UART0->C2 &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK);
    
    sbr = (uint16_t)((SYSTEM_CLOCK) / (UART_BAUD_RATE * UART_OVERSAMPLE_RATE));
    UART0->BDH = (UART0->BDH & ~UART_BDH_SBR_MASK) | ((sbr >> 8) & 0x1F);
    UART0->BDL = (uint8_t)(sbr & 0xFF);
    
    UART0->C1 = 0;
    UART0->C4 = UART_C4_OSR(UART_OVERSAMPLE_RATE - 1);
    
    UART0->C2 |= UART_C2_RIE_MASK;
    
    NVIC_SetPriority(UART0_IRQn, 2);
    NVIC_ClearPendingIRQ(UART0_IRQn);
    NVIC_EnableIRQ(UART0_IRQn);
    
    UART0->C2 |= (UART_C2_TE_MASK | UART_C2_RE_MASK);
}

void UART_SendChar(char c) {
    while (!(UART0->S1 & UART_S1_TDRE_MASK));
    UART0->D = c;
}

void UART_SendString(const char *str) {
    while (*str) {
        UART_SendChar(*str++);
    }
}

void UART_SendStatus(SystemStatus_t *status) {
    char buffer[128];
    
    sprintf(buffer, "{\"soil\":%d,\"light\":%d,\"water\":%d,\"pump\":%d,\"led\":%d}\n",
            status->soilMoisture,
            status->lightLevel,
            status->waterLevel,
            status->pumpStatus,
            status->ledStatus);
    
    UART_SendString(buffer);
}

Command_t UART_ReceiveCommand(uint8_t *data) {
    if (!commandReady) {
        return CMD_NONE;
    }
    
    Command_t cmd = CMD_NONE;
    
    if (rxBuffer[0] == 'P' && rxBuffer[1] == '1') {
        cmd = CMD_PUMP_ON;
    } else if (rxBuffer[0] == 'P' && rxBuffer[1] == '0') {
        cmd = CMD_PUMP_OFF;
    } else if (rxBuffer[0] == 'L' && rxBuffer[1] == '1') {
        cmd = CMD_LED_ON;
    } else if (rxBuffer[0] == 'L' && rxBuffer[1] == '0') {
        cmd = CMD_LED_OFF;
    } else if (rxBuffer[0] == 'S' && rxBuffer[1] == 'T') {
        cmd = CMD_SET_SOIL_THRESHOLD;
        if (data) {
            *data = rxBuffer[2];
        }
    } else if (rxBuffer[0] == 'L' && rxBuffer[1] == 'T') {
        cmd = CMD_SET_LIGHT_THRESHOLD;
        if (data) {
            *data = rxBuffer[2];
        }
    }
    
    commandReady = 0;
    rxIndex = 0;
    
    return cmd;
}

void UART0_IRQHandler(void) {
    if (UART0->S1 & UART_S1_RDRF_MASK) {
        uint8_t data = UART0->D;
        
        if (data == '\n' || data == '\r') {
            if (rxIndex > 0) {
                rxBuffer[rxIndex] = '\0';
                commandReady = 1;
            }
        } else {
            if (rxIndex < sizeof(rxBuffer) - 1) {
                rxBuffer[rxIndex++] = data;
            }
        }
    }
}
