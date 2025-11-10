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
    
    WebServer_UpdateStatus(&systemStatus);
    
    delay(100);
}

void parseUARTData(String data) {
    data.trim();
    
    if (data == "PUMP_ON_MANUAL") {
        systemStatus.pumpStatus = 1;
        systemStatus.pumpMode = 1;
        Serial.println("PUMP turned ON (MANUAL)");
    }
    else if (data == "PUMP_OFF_MANUAL") {
        systemStatus.pumpStatus = 0;
        systemStatus.pumpMode = 1;
        Serial.println("PUMP turned OFF (MANUAL)");
    }
    else if (data == "PUMP_ON_AUTO") {
        systemStatus.pumpStatus = 1;
        systemStatus.pumpMode = 0;
        Serial.println("PUMP turned ON (AUTO)");
    }
    else if (data == "PUMP_OFF_AUTO") {
        systemStatus.pumpStatus = 0;
        systemStatus.pumpMode = 0;
        Serial.println("PUMP turned OFF (AUTO)");
    }
    else if (data == "LIGHT_ON_MANUAL") {
        systemStatus.ledStatus = 1;
        systemStatus.lightMode = 1;
        Serial.println("LIGHT turned ON (MANUAL)");
    }
    else if (data == "LIGHT_OFF_MANUAL") {
        systemStatus.ledStatus = 0;
        systemStatus.lightMode = 1;
        Serial.println("LIGHT turned OFF (MANUAL)");
    }
    else if (data == "LIGHT_ON_AUTO") {
        systemStatus.ledStatus = 1;
        systemStatus.lightMode = 0;
        Serial.println("LIGHT turned ON (AUTO)");
    }
    else if (data == "LIGHT_OFF_AUTO") {
        systemStatus.ledStatus = 0;
        systemStatus.lightMode = 0;
        Serial.println("LIGHT turned OFF (AUTO)");
    }
    else if (data.startsWith("WL:")) {
        Serial.print("Water Level: ");
        Serial.println(data);
    }
    else {
        Serial.print("MCXC444: ");
        Serial.println(data);
    }
}
