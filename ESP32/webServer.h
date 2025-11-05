#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdint.h>

typedef struct {
    uint8_t soilMoisture;
    uint8_t lightLevel;
    uint8_t waterLevel;
    uint8_t pumpStatus;
    uint8_t ledStatus;
} SystemStatus_t;

void WebServer_Init(void);
void WebServer_HandleClient(void);
void WebServer_UpdateStatus(SystemStatus_t *status);
const char* WebServer_GetSSID(void);
const char* WebServer_GetPassword(void);
const char* WebServer_GetIPAddress(void);

#endif
