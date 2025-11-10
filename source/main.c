#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "board.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "includes/led.h"
#include "includes/soilMoisture.h"
#include "includes/lightSensor.h"
#include "includes/waterPump.h"
#include "includes/uart.h"

#define LDR_POLL_MS 20
#define DEBOUNCE_MS 30
#define PUMP_MIN_HOLD_MS 500

static volatile int lastSoilState = 0;
static volatile bool manualLightControl = false;
static volatile bool manualPumpControl = false;

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    while(1);
}

void vApplicationMallocFailedHook(void) {
    while(1);
}

static void TaskLDRPoll(void *arg) {
    (void)arg;
    int prev = -1;

    for (;;) {
        if (!manualLightControl) {
            int ldrState = LightSensor_Read();

            if (ldrState != prev) {
                if (ldrState) {
                    LED_AllOn();
                } else {
                    LED_AllOff();
                }
                prev = ldrState;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(LDR_POLL_MS));
    }
}

static void TaskPumpControl(void *arg) {
    (void)arg;

    const int LEVEL_WET = 0;
    const int LEVEL_DRY = 1 - LEVEL_WET;

    SemaphoreHandle_t soilSem = SoilMoisture_GetSemaphore();

    for (;;) {
        xSemaphoreTake(soilSem, portMAX_DELAY);

        if (!manualPumpControl) {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            int s1 = SoilMoisture_Read();
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            int s2 = SoilMoisture_Read();

            if (s1 == s2 && s2 != lastSoilState) {
                lastSoilState = s2;

                if (lastSoilState == LEVEL_DRY) {
                    WaterPump_On();
                    PRINTF("Pump ON (dry)\r\n");
                } else {
                    WaterPump_Off();
                    PRINTF("Pump OFF (wet)\r\n");
                }

                vTaskDelay(pdMS_TO_TICKS(PUMP_MIN_HOLD_MS));
            }
        }
    }
}

static void sendStatusData(void) {
    char statusMessage[MAX_MSG_LEN];
    bool relayState = WaterPump_GetState();
    bool lightState = LED_GetState();

    if (relayState) {
        snprintf(statusMessage, MAX_MSG_LEN, manualPumpControl ? "PUMP_ON_MANUAL" : "PUMP_ON_AUTO");
        UART_SendMessage(statusMessage);
    } else {
        snprintf(statusMessage, MAX_MSG_LEN, manualPumpControl ? "PUMP_OFF_MANUAL" : "PUMP_OFF_AUTO");
        UART_SendMessage(statusMessage);
    }

    if (lightState) {
        snprintf(statusMessage, MAX_MSG_LEN, manualLightControl ? "LIGHT_ON_MANUAL" : "LIGHT_ON_AUTO");
        UART_SendMessage(statusMessage);
    } else {
        snprintf(statusMessage, MAX_MSG_LEN, manualLightControl ? "LIGHT_OFF_MANUAL" : "LIGHT_OFF_AUTO");
        UART_SendMessage(statusMessage);
    }
}

static void recvTask(void *p) {
    QueueHandle_t queue = UART_GetQueue();
    
    while(1) {
        UARTMessage_t msg;
        if (xQueueReceive(queue, (UARTMessage_t *)&msg, portMAX_DELAY) == pdTRUE) {
            PRINTF("Received: [%s] (len=%d)\r\n", msg.message, strlen(msg.message));

            if (strncmp(msg.message, "WL:", 3) == 0) {
                int waterLevel = atoi(msg.message + 3);
                if (waterLevel < 6000) {
                    PRINTF("Water level too low: %d\r\n", waterLevel);
                } else {
                    PRINTF("Water level is sufficient: %d\r\n", waterLevel);
                }
            } else if (strcmp(msg.message, "PUMP_ON") == 0) {
                manualPumpControl = true;
                WaterPump_On();
                PRINTF(">> Pump turned ON (MANUAL mode)\r\n");
                sendStatusData();
            } else if (strcmp(msg.message, "PUMP_OFF") == 0) {
                manualPumpControl = true;
                WaterPump_Off();
                PRINTF(">> Pump turned OFF (MANUAL mode)\r\n");
                sendStatusData();
            } else if (strcmp(msg.message, "PUMP_AUTO") == 0) {
                manualPumpControl = false;
                PRINTF(">> Pump control set to AUTO mode\r\n");
                sendStatusData();
            } else if (strcmp(msg.message, "LIGHT_ON") == 0) {
                manualLightControl = true;
                LED_AllOn();
                PRINTF(">> Light turned ON (MANUAL mode)\r\n");
                sendStatusData();
            } else if (strcmp(msg.message, "LIGHT_OFF") == 0) {
                manualLightControl = true;
                LED_AllOff();
                PRINTF(">> Light turned OFF (MANUAL mode)\r\n");
                sendStatusData();
            } else if (strcmp(msg.message, "LIGHT_AUTO") == 0) {
                manualLightControl = false;
                PRINTF(">> Light control set to AUTO mode\r\n");
                sendStatusData();
            } else {
                PRINTF("Unknown command\r\n");
            }
        }
    }
}

static void sendTask(void *p) {
    while (1) {
        sendStatusData();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    LED_Init();
    LED_AllOff();

    WaterPump_Init();
    WaterPump_Off();

    LightSensor_Init();
    SoilMoisture_Init();

    UART_Init(9600);
    PRINTF("UART2 DEMO\r\n");

    lastSoilState = SoilMoisture_Read();

    xTaskCreate(TaskLDRPoll, "LDR", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(TaskPumpControl, "PUMP", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(recvTask, "recvTask", configMINIMAL_STACK_SIZE+100, NULL, 2, NULL);
    xTaskCreate(sendTask, "sendTask", configMINIMAL_STACK_SIZE+100, NULL, 1, NULL);

    vTaskStartScheduler();

    volatile static int i = 0 ;
    while(1) {
        i++ ;
        __asm volatile ("nop");
    }
    return 0 ;
}
