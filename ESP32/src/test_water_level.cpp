// #include <Arduino.h>
// #include "waterLevel.h"

// #define WATER_LEVEL_PIN 16
// #define ADC_MAX_VALUE 4095

// void setup() {
//     Serial.begin(115200);
//     while (!Serial) delay(10);

//     Serial.println("\n--- Water Level Sensor Test ---");
//     WaterLevel_Init();
//     delay(100);
// }

// static uint8_t waterLowThreshold = WATER_LOW_THRESHOLD;

// void WaterLevel_Init(void) {
//     pinMode(WATER_LEVEL_PIN, INPUT);
//     analogSetAttenuation(ADC_11db);
// }

// uint16_t WaterLevel_ReadRaw(void) {
//     return analogRead(WATER_LEVEL_PIN);
// }

// uint8_t WaterLevel_GetPercentage(void) {
//     uint16_t raw = WaterLevel_ReadRaw();
//     uint8_t percentage = (uint8_t)((raw * 100) / ADC_MAX_VALUE);
//     return percentage;
// }

// uint8_t WaterLevel_IsLow(void) {
//     return (WaterLevel_GetPercentage() < waterLowThreshold);
// }

// void loop() {
//     uint16_t raw = WaterLevel_ReadRaw();
//     uint8_t percent = WaterLevel_GetPercentage();

//     Serial.printf("Raw: %u\tPercentage: %u%%\n", raw, percent);

//     delay(1000);
// }
