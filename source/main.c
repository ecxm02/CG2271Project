#include "board.h"
#include "pin_mux.h"
#include "FreeRTOS.h"
#include "task.h"
#include "includes/led.h"
#include "includes/soilMoisture.h"
#include "includes/lightSensor.h"
#include "includes/waterPump.h"

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    while(1);
}

void vApplicationMallocFailedHook(void) {
    while(1);
}

void LightSensorTask(void *pvParameters) {
    while(1) {
        // Poll the LDR (light sensor) to control the LEDs
        // Mimics Melvin's pollLDR() logic
        uint8_t ldrState = LightSensor_Read();
        
        if (ldrState != 0) {
            LED_AllOn();  // Turn all LEDs on when light detected
        } else {
            LED_AllOff();  // Turn all LEDs off when dark
        }
        
        // Small delay for polling rate control
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void IdleTask(void *pvParameters) {
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    
    LED_Init();
    WaterPump_Init();
    SoilMoisture_Init();
    LightSensor_Init();
    
    xTaskCreate(LightSensorTask, "LightSensor", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(IdleTask, "Idle", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    
    vTaskStartScheduler();
    
    while(1);
}
