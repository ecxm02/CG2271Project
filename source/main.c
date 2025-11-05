#include "board.h"
#include "pin_mux.h"
#include "FreeRTOS.h"
#include "task.h"
#include "led.h"
#include "soilMoisture.h"
#include "lightSensor.h"
#include "waterPump.h"

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
    
    xTaskCreate(IdleTask, "Idle", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    
    vTaskStartScheduler();
    
    while(1);
}
