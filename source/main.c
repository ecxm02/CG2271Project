#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "led.h"
#include "soilMoisture.h"
#include "lightSensor.h"
#include "waterPump.h"
#include "uart_comm.h"
#include "buzzer.h"

#define UPDATE_INTERVAL_MS 1000
#define MIN_WATER_LEVEL 20
#define MAX_PUMP_DURATION_MS 5000

#define SENSOR_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define CONTROL_TASK_PRIORITY (tskIDLE_PRIORITY + 3)
#define UART_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define SAFETY_TASK_PRIORITY (tskIDLE_PRIORITY + 4)

#define SENSOR_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 2)
#define CONTROL_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 2)
#define UART_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 3)
#define SAFETY_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE)

typedef enum {
    MODE_AUTO = 0,
    MODE_MANUAL = 1
} SystemMode_t;

typedef struct {
    uint8_t soilMoisture;
    uint8_t lightLevel;
    uint8_t waterLevel;
} SensorData_t;

static SystemMode_t systemMode = MODE_AUTO;
static uint8_t waterLevel = 100;
static TickType_t pumpStartTicks = 0;

static QueueHandle_t sensorDataQueue;
static QueueHandle_t commandQueue;
static SemaphoreHandle_t systemModeMutex;
static SemaphoreHandle_t pumpControlMutex;

void SensorTask(void *pvParameters);
void ControlTask(void *pvParameters);
void UARTTask(void *pvParameters);
void SafetyTask(void *pvParameters);

int main(void) {
    LED_Init();
    SoilMoisture_Init();
    LightSensor_Init();
    WaterPump_Init();
    Buzzer_Init();
    UART_Init();
    
    LED_On(LED_RED);
    vTaskDelay(pdMS_TO_TICKS(200));
    LED_Off(LED_RED);
    LED_On(LED_GREEN);
    vTaskDelay(pdMS_TO_TICKS(200));
    LED_Off(LED_GREEN);
    LED_On(LED_BLUE);
    vTaskDelay(pdMS_TO_TICKS(200));
    LED_AllOff();
    
    sensorDataQueue = xQueueCreate(5, sizeof(SensorData_t));
    commandQueue = xQueueCreate(10, sizeof(Command_t));
    systemModeMutex = xSemaphoreCreateMutex();
    pumpControlMutex = xSemaphoreCreateMutex();
    
    if (sensorDataQueue == NULL || commandQueue == NULL || 
        systemModeMutex == NULL || pumpControlMutex == NULL) {
        UART_SendString("ERROR: Failed to create FreeRTOS objects\n");
        while(1);
    }
    
    xTaskCreate(SensorTask, "Sensor", SENSOR_TASK_STACK_SIZE, NULL, SENSOR_TASK_PRIORITY, NULL);
    xTaskCreate(ControlTask, "Control", CONTROL_TASK_STACK_SIZE, NULL, CONTROL_TASK_PRIORITY, NULL);
    xTaskCreate(UARTTask, "UART", UART_TASK_STACK_SIZE, NULL, UART_TASK_PRIORITY, NULL);
    xTaskCreate(SafetyTask, "Safety", SAFETY_TASK_STACK_SIZE, NULL, SAFETY_TASK_PRIORITY, NULL);
    
    UART_SendString("FreeRTOS Plant Watering System Started\n");
    
    vTaskStartScheduler();
    
    while(1);
}

void SensorTask(void *pvParameters) {
    SensorData_t sensorData;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100);
    
    while(1) {
        sensorData.soilMoisture = SoilMoisture_Read();
        sensorData.lightLevel = LightSensor_Read();
        sensorData.waterLevel = waterLevel;
        
        if (xQueueSend(sensorDataQueue, &sensorData, 0) != pdPASS) {
            UART_SendString("WARNING: Sensor queue full\n");
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void ControlTask(void *pvParameters) {
    SensorData_t sensorData;
    SystemMode_t currentMode;
    
    while(1) {
        if (xQueueReceive(sensorDataQueue, &sensorData, portMAX_DELAY) == pdPASS) {
            if (xSemaphoreTake(systemModeMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                currentMode = systemMode;
                xSemaphoreGive(systemModeMutex);
                
                if (sensorData.waterLevel < MIN_WATER_LEVEL) {
                    Buzzer_On();
                    LED_On(LED_RED);
                } else {
                    Buzzer_Off();
                    LED_Off(LED_RED);
                }
                
                if (currentMode == MODE_AUTO) {
                    if (LightSensor_IsDark()) {
                        LED_On(LED_BLUE);
                    } else {
                        LED_Off(LED_BLUE);
                    }
                    
                    if (xSemaphoreTake(pumpControlMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        if (SoilMoisture_IsDry() && sensorData.waterLevel > MIN_WATER_LEVEL) {
                            if (!WaterPump_GetStatus()) {
                                WaterPump_On();
                                pumpStartTicks = xTaskGetTickCount();
                                LED_On(LED_GREEN);
                            }
                        } else {
                            WaterPump_Off();
                            LED_Off(LED_GREEN);
                        }
                        xSemaphoreGive(pumpControlMutex);
                    }
                }
            }
        }
    }
}

void UARTTask(void *pvParameters) {
    SystemStatus_t status;
    Command_t cmd;
    uint8_t cmdData;
    TickType_t xLastStatusTime = xTaskGetTickCount();
    const TickType_t xStatusInterval = pdMS_TO_TICKS(UPDATE_INTERVAL_MS);
    
    while(1) {
        cmd = UART_ReceiveCommand(&cmdData);
        
        if (cmd != CMD_NONE) {
            if (xSemaphoreTake(systemModeMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                switch (cmd) {
                    case CMD_PUMP_ON:
                        systemMode = MODE_MANUAL;
                        if (xSemaphoreTake(pumpControlMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                            WaterPump_On();
                            pumpStartTicks = xTaskGetTickCount();
                            xSemaphoreGive(pumpControlMutex);
                        }
                        UART_SendString("Pump ON (Manual)\n");
                        break;
                        
                    case CMD_PUMP_OFF:
                        if (xSemaphoreTake(pumpControlMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                            WaterPump_Off();
                            xSemaphoreGive(pumpControlMutex);
                        }
                        UART_SendString("Pump OFF (Manual)\n");
                        break;
                        
                    case CMD_LED_ON:
                        systemMode = MODE_MANUAL;
                        LED_On(LED_BLUE);
                        UART_SendString("LED ON (Manual)\n");
                        break;
                        
                    case CMD_LED_OFF:
                        LED_Off(LED_BLUE);
                        UART_SendString("LED OFF (Manual)\n");
                        break;
                        
                    case CMD_AUTO_MODE:
                        systemMode = MODE_AUTO;
                        UART_SendString("Auto Mode Enabled\n");
                        break;
                        
                    case CMD_SET_WATER_LEVEL:
                        waterLevel = cmdData;
                        break;
                        
                    default:
                        break;
                }
                xSemaphoreGive(systemModeMutex);
            }
        }
        
        if ((xTaskGetTickCount() - xLastStatusTime) >= xStatusInterval) {
            status.soilMoisture = SoilMoisture_Read();
            status.lightLevel = LightSensor_Read();
            status.waterLevel = waterLevel;
            status.pumpStatus = WaterPump_GetStatus();
            status.ledStatus = LED_GetStatus(LED_BLUE);
            
            UART_SendStatus(&status);
            xLastStatusTime = xTaskGetTickCount();
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void SafetyTask(void *pvParameters) {
    TickType_t currentTicks;
    
    while(1) {
        if (xSemaphoreTake(pumpControlMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (WaterPump_GetStatus()) {
                currentTicks = xTaskGetTickCount();
                
                if ((currentTicks - pumpStartTicks) > pdMS_TO_TICKS(MAX_PUMP_DURATION_MS)) {
                    WaterPump_Off();
                    UART_SendString("SAFETY: Pump auto-off (timeout)\n");
                }
            }
            xSemaphoreGive(pumpControlMutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    UART_SendString("ERROR: Stack overflow in task: ");
    UART_SendString(pcTaskName);
    UART_SendString("\n");
    while(1);
}

void vApplicationMallocFailedHook(void) {
    UART_SendString("ERROR: Malloc failed\n");
    while(1);
}
