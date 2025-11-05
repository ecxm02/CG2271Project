#ifndef UART_COMM_H
#define UART_COMM_H

#include <stdint.h>

#define UART_BAUD_RATE 9600

typedef struct {
    uint8_t soilMoisture;
    uint8_t lightLevel;
    uint8_t waterLevel;
    uint8_t pumpStatus;
    uint8_t ledStatus;
} SystemStatus_t;

typedef enum {
    CMD_NONE = 0,
    CMD_PUMP_ON = 1,
    CMD_PUMP_OFF = 2,
    CMD_LED_ON = 3,
    CMD_LED_OFF = 4,
    CMD_SET_SOIL_THRESHOLD = 5,
    CMD_SET_LIGHT_THRESHOLD = 6,
    CMD_SET_WATER_LEVEL = 7,
    CMD_AUTO_MODE = 8
} Command_t;

void UART_Init(void);
void UART_SendStatus(SystemStatus_t *status);
Command_t UART_ReceiveCommand(uint8_t *data);
void UART_SendChar(char c);
void UART_SendString(const char *str);

#endif
