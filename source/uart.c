#include "includes/uart.h"
#include "board.h"
#include "fsl_debug_console.h"
#include <string.h>

#define BAUD_RATE 9600
#define UART_TX_PTE22 22
#define UART_RX_PTE23 23
#define UART2_INT_PRIO 128

static char send_buffer[MAX_MSG_LEN];
static volatile int send_ptr = 0;
static QueueHandle_t uart_queue = NULL;

void UART_Init(uint32_t baud_rate) {
    NVIC_DisableIRQ(UART2_FLEXIO_IRQn);

    SIM->SCGC4 |= SIM_SCGC4_UART2_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

    UART2->C2 &= ~((UART_C2_TE_MASK) | (UART_C2_RE_MASK));

    PORTE->PCR[UART_TX_PTE22] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[UART_TX_PTE22] |= PORT_PCR_MUX(4);

    PORTE->PCR[UART_RX_PTE23] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[UART_RX_PTE23] |= PORT_PCR_MUX(4);

    uint32_t bus_clk = CLOCK_GetBusClkFreq();
    uint32_t sbr = (bus_clk + (baud_rate * 8)) / (baud_rate * 16);

    UART2->BDH &= ~UART_BDH_SBR_MASK;
    UART2->BDH |= ((sbr >> 8) & UART_BDH_SBR_MASK);
    UART2->BDL = (uint8_t)(sbr & 0xFF);

    UART2->C1 &= ~UART_C1_LOOPS_MASK;
    UART2->C1 &= ~UART_C1_RSRC_MASK;

    UART2->C1 &= ~UART_C1_PE_MASK;

    UART2->C1 &= ~UART_C1_M_MASK;

    UART2->C2 |= UART_C2_RIE_MASK;

    UART2->C2 |= UART_C2_RE_MASK;

    NVIC_SetPriority(UART2_FLEXIO_IRQn, UART2_INT_PRIO);
    NVIC_ClearPendingIRQ(UART2_FLEXIO_IRQn);
    NVIC_EnableIRQ(UART2_FLEXIO_IRQn);

    uart_queue = xQueueCreate(UART_QUEUE_LENGTH, sizeof(UARTMessage_t));
}

void UART_SendMessage(char *message) {
    // Wait for previous transmission to complete
    while (send_ptr != 0) {
        // Busy wait
    }
    
    int len = strlen(message);
    strncpy(send_buffer, message, MAX_MSG_LEN - 2);
    send_buffer[len] = '\r';
    send_buffer[len + 1] = '\n';
    send_buffer[len + 2] = '\0';
    
    // Debug: print what we're actually sending
    for(int i = 0; i < len + 2; i++) {
        if(send_buffer[i] == '\r') PRINTF("[CR]");
        else if(send_buffer[i] == '\n') PRINTF("[LF]");
        else PRINTF("%c", send_buffer[i]);
    }
    PRINTF("\r\n");

    send_ptr = 0;  // Reset pointer before starting transmission
    UART2->C2 |= UART_C2_TE_MASK;
    UART2->C2 |= UART_C2_TIE_MASK;
}

QueueHandle_t UART_GetQueue(void) {
    return uart_queue;
}

void UART2_FLEXIO_IRQHandler(void) {
    static int recv_ptr = 0;
    char rx_data;
    static char recv_buffer[MAX_MSG_LEN];

    if (UART2->S1 & UART_S1_TDRE_MASK) {
        if (send_buffer[send_ptr] == '\0') {
            UART2->C2 &= ~UART_C2_TIE_MASK;
            UART2->C2 &= ~UART_C2_TE_MASK;
            send_ptr = 0;
        } else {
            UART2->D = send_buffer[send_ptr++];
        }
    }

    if (UART2->S1 & UART_S1_RDRF_MASK) {
        UARTMessage_t msg;
        rx_data = UART2->D;
        
        if (rx_data == '\n') {
            BaseType_t hpw;
            recv_buffer[recv_ptr] = '\0';
            
            if (recv_ptr > 0) {
                if (recv_buffer[recv_ptr - 1] == '\r') {
                    recv_buffer[recv_ptr - 1] = '\0';
                }
                strncpy(msg.message, recv_buffer, MAX_MSG_LEN);
                xQueueSendFromISR(uart_queue, (void *)&msg, &hpw);
                portYIELD_FROM_ISR(hpw);
            }
            recv_ptr = 0;
        } else {
            if (recv_ptr < MAX_MSG_LEN - 1) {
                recv_buffer[recv_ptr++] = rx_data;
            }
        }
    }
}
