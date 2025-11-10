#include <Arduino.h>
#include "waterLevel.h"
#include "webServer.h"

#define RX_PIN 2
#define TX_PIN 1
#define SERIAL_BAUD 9600
#define WATER_SENSOR_PIN 4

SystemStatus_t systemStatus = {0};
String uartBuffer = "";

void parseUARTData(String data);

void setup() {
    Serial.begin(115200);
    Serial1.begin(SERIAL_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
    analogReadResolution(12);
    
    WebServer_Init();
    
    Serial.println("\n=== ESP32 Plant Watering System ===");
    Serial.printf("Connect to WiFi: %s\n", WebServer_GetSSID());
    Serial.printf("Password: %s\n", WebServer_GetPassword());
    Serial.printf("Open browser to: http://%s\n", WebServer_GetIPAddress());
    Serial.println("====================================\n");
}

void loop() {
    WebServer_HandleClient();
    
    while (Serial1.available()) {
        char c = Serial1.read();
        
        if (c == '\n') {
            if (uartBuffer.length() > 0) {
                parseUARTData(uartBuffer);
                uartBuffer = "";
            }
        } else if (c != '\r') {
            uartBuffer += c;
        }
    }
    
    int waterLevelRaw = analogRead(WATER_SENSOR_PIN);
    systemStatus.waterLevel = waterLevelRaw;
    
    String waterLevelStr = "WL:" + String(waterLevelRaw);
    Serial1.println(waterLevelStr);
    
    WebServer_UpdateStatus(&systemStatus);
    
    delay(100);
}

void parseUARTData(String data) {
    data.trim();
    
    if (data == "PUMP_ON") {
        systemStatus.pumpStatus = 1;
        Serial.println("PUMP turned ON");
    } 
    else if (data == "PUMP_OFF") {
        systemStatus.pumpStatus = 0;
        Serial.println("PUMP turned OFF");
    } 
    else if (data == "LIGHT_ON") {
        systemStatus.ledStatus = 1;
        Serial.println("LIGHT turned ON");
    } 
    else if (data == "LIGHT_OFF") {
        systemStatus.ledStatus = 0;
        Serial.println("LIGHT turned OFF");
    } 
    else if (data.startsWith("Relay")) {
        if (data.indexOf("Relay ON") != -1) {
            systemStatus.pumpStatus = 1;
        } else {
            systemStatus.pumpStatus = 0;
        }
        
        if (data.indexOf("Light ON") != -1) {
            systemStatus.ledStatus = 1;
        } else {
            systemStatus.ledStatus = 0;
        }
        
        Serial.print("Status: ");
        Serial.println(data);
    }
    else {
        Serial.print("MCXC444: ");
        Serial.println(data);
    }
}
