#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define PROTOCOL_VERSION 1

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
    CMD_SET_LIGHT_THRESHOLD = 6
} Command_t;

#define CMD_PUMP_ON_STR "P1"
#define CMD_PUMP_OFF_STR "P0"
#define CMD_LED_ON_STR "L1"
#define CMD_LED_OFF_STR "L0"
#define CMD_SOIL_THRESHOLD_STR "ST"
#define CMD_LIGHT_THRESHOLD_STR "LT"

#endif
