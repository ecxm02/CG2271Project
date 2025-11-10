#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"

#define MAX_MSG_LEN 256
#define UART_QUEUE_LENGTH 5

typedef struct {
    char message[MAX_MSG_LEN];
} UARTMessage_t;

void UART_Init(uint32_t baud_rate);
void UART_SendMessage(char *message);
QueueHandle_t UART_GetQueue(void);

#endif
