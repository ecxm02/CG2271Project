#include <Arduino.h>
#include "waterLevel.h"
#include "webServer.h"

#define RX_PIN 16
#define TX_PIN 17
#define SERIAL_BAUD 9600
#define UPDATE_INTERVAL 1000

SystemStatus_t systemStatus = {0};
unsigned long lastUpdate = 0;
String uartBuffer = "";

void parseUARTData(String data);

void setup() {
    Serial.begin(115200);
    Serial2.begin(SERIAL_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
    
    WaterLevel_Init();
    WebServer_Init();
    
    Serial.println("\n=== ESP32 Plant Watering System ===");
    Serial.printf("Connect to WiFi: %s\n", WebServer_GetSSID());
    Serial.printf("Password: %s\n", WebServer_GetPassword());
    Serial.printf("Open browser to: http://%s\n", WebServer_GetIPAddress());
    Serial.println("====================================\n");
}

void loop() {
    WebServer_HandleClient();
    
    while (Serial2.available()) {
        char c = Serial2.read();
        
        if (c == '\n') {
            if (uartBuffer.length() > 0) {
                parseUARTData(uartBuffer);
                uartBuffer = "";
            }
        } else if (c != '\r') {
            uartBuffer += c;
        }
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastUpdate >= UPDATE_INTERVAL) {
        systemStatus.waterLevel = WaterLevel_GetPercentage();
        
        WebServer_UpdateStatus(&systemStatus);
        
        lastUpdate = currentTime;
    }
}

void parseUARTData(String data) {
    if (data.startsWith("{")) {
        int soilIdx = data.indexOf("\"soil\":");
        int lightIdx = data.indexOf("\"light\":");
        int waterIdx = data.indexOf("\"water\":");
        int pumpIdx = data.indexOf("\"pump\":");
        int ledIdx = data.indexOf("\"led\":");
        
        if (soilIdx != -1) {
            systemStatus.soilMoisture = data.substring(soilIdx + 7).toInt();
        }
        if (lightIdx != -1) {
            systemStatus.lightLevel = data.substring(lightIdx + 8).toInt();
        }
        if (waterIdx != -1) {
            systemStatus.waterLevel = data.substring(waterIdx + 8).toInt();
        }
        if (pumpIdx != -1) {
            systemStatus.pumpStatus = data.substring(pumpIdx + 7).toInt();
        }
        if (ledIdx != -1) {
            systemStatus.ledStatus = data.substring(ledIdx + 6).toInt();
        }
        
        Serial.print("Status Update - Soil:");
        Serial.print(systemStatus.soilMoisture);
        Serial.print("% Light:");
        Serial.print(systemStatus.lightLevel);
        Serial.print("% Water:");
        Serial.print(systemStatus.waterLevel);
        Serial.print("% Pump:");
        Serial.print(systemStatus.pumpStatus ? "ON" : "OFF");
        Serial.print(" LED:");
        Serial.println(systemStatus.ledStatus ? "ON" : "OFF");
    } else {
        Serial.print("MCXC444: ");
        Serial.println(data);
    }
}
