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
#define PUMP_MIN_HOLD_MS 100

static volatile int lastSoilState = 0;
static volatile bool manualLightControl = false;
static volatile bool manualPumpControl = false;
static volatile bool waterLevelLow = false;

static void sendStatusData(void);

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

                if (lastSoilState == LEVEL_DRY && !waterLevelLow) {
                    WaterPump_On();
                } else {
                    WaterPump_Off();
                }

                vTaskDelay(pdMS_TO_TICKS(PUMP_MIN_HOLD_MS));
            }
        }
        
        // Safety check: turn off pump if water level is low
        if (waterLevelLow && WaterPump_GetState()) {
            WaterPump_Off();
            PRINTF("   -> Pump OFF (low water safety)\r\n");
        }
    }
}

static void sendStatusData(void) {
    char statusMessage[MAX_MSG_LEN];
    bool relayState = WaterPump_GetState();
    bool lightState = LED_GetState();

    snprintf(statusMessage, MAX_MSG_LEN, "STATUS:%d:%d:%d:%d", 
             relayState ? 1 : 0, 
             manualPumpControl ? 1 : 0,
             lightState ? 1 : 0, 
             manualLightControl ? 1 : 0);
    
    PRINTF("TX -> %s\\r\\n\r\n", statusMessage);
    UART_SendMessage(statusMessage);
}

static void recvTask(void *p) {
    QueueHandle_t queue = UART_GetQueue();
    
    while(1) {
        UARTMessage_t msg;
        if (xQueueReceive(queue, (UARTMessage_t *)&msg, portMAX_DELAY) == pdTRUE) {
            PRINTF("RX <- [%s]\r\n", msg.message);
            
            if (strcmp(msg.message, "PUMP_ON") == 0) {
                manualPumpControl = true;
                if (!waterLevelLow) {
                    WaterPump_On();
                    PRINTF("   -> Pump ON (MANUAL)\r\n");
                } else {
                    WaterPump_Off();
                    PRINTF("   -> Pump remains OFF (low water)\r\n");
                }
            } else if (strcmp(msg.message, "PUMP_OFF") == 0) {
                manualPumpControl = true;
                WaterPump_Off();
                PRINTF("   -> Pump OFF (MANUAL)\r\n");
            } else if (strcmp(msg.message, "PUMP_AUTO") == 0) {
                manualPumpControl = false;
                PRINTF("   -> Pump mode AUTO\r\n");
            } else if (strcmp(msg.message, "WATER_LOW") == 0) {
                waterLevelLow = true;
                WaterPump_Off();
                PRINTF("   -> Water level LOW - pump locked OFF\r\n");
            } else if (strcmp(msg.message, "WATER_OK") == 0) {
                waterLevelLow = false;
                PRINTF("   -> Water level OK - pump unlocked\r\n");
                // Wake up TaskPumpControl to re-evaluate pump state
                if (!manualPumpControl && lastSoilState == 1) {
                    SemaphoreHandle_t soilSem = SoilMoisture_GetSemaphore();
                    xSemaphoreGive(soilSem);
                }
            } else if (strcmp(msg.message, "LIGHT_ON") == 0) {
                manualLightControl = true;
                LED_AllOn();
                PRINTF("   -> Light ON (MANUAL)\r\n");
            } else if (strcmp(msg.message, "LIGHT_OFF") == 0) {
                manualLightControl = true;
                LED_AllOff();
                PRINTF("   -> Light OFF (MANUAL)\r\n");
            } else if (strcmp(msg.message, "LIGHT_AUTO") == 0) {
                manualLightControl = false;
                PRINTF("   -> Light mode AUTO\r\n");
            } else {
                PRINTF("   -> Unknown command\r\n");
            }
        }
    }
}

static void sendTask(void *p) {
    while (1) {
        PRINTF("sendTask tick\r\n");
        sendStatusData();
        vTaskDelay(pdMS_TO_TICKS(100));
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
