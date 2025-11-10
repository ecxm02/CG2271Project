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
    
    if (Serial1.available()) {
        Serial.println("UART data available!");
    }
    
    while (Serial1.available()) {
        char c = Serial1.read();
        Serial.printf("Char: %c (0x%02X)\n", c >= 32 ? c : '?', c);
        
        if (c == '\n') {
            Serial.println(">>> GOT NEWLINE!");
            if (uartBuffer.length() > 0) {
                Serial.printf("Complete message: [%s]\n", uartBuffer.c_str());
                parseUARTData(uartBuffer);
                uartBuffer = "";
            }
        } else if (c != '\r') {
            uartBuffer += c;
        } else {
            Serial.println(">>> GOT CARRIAGE RETURN (ignored)");
        }
    }
    
    int waterLevelRaw = analogRead(WATER_SENSOR_PIN);
    systemStatus.waterLevel = waterLevelRaw;
    
    delay(100);
}

void parseUARTData(String data) {
    data.trim();
    
    if (data.length() == 0) {
        return;
    }
    
    Serial.print("RX: ");
    Serial.println(data);
    
    if (data.startsWith("STATUS:")) {
        // Format: STATUS:pumpStatus:pumpMode:lightStatus:lightMode
        int idx = 7; // Skip "STATUS:"
        int colon1 = data.indexOf(':', idx);
        int colon2 = data.indexOf(':', colon1 + 1);
        int colon3 = data.indexOf(':', colon2 + 1);
        
        systemStatus.pumpStatus = data.substring(idx, colon1).toInt();
        systemStatus.pumpMode = data.substring(colon1 + 1, colon2).toInt();
        systemStatus.ledStatus = data.substring(colon2 + 1, colon3).toInt();
        systemStatus.lightMode = data.substring(colon3 + 1).toInt();
        
        Serial.printf("Parsed -> P:%d(%d) L:%d(%d)\n", 
                     systemStatus.pumpStatus, systemStatus.pumpMode,
                     systemStatus.ledStatus, systemStatus.lightMode);
        
        WebServer_UpdateStatus(&systemStatus);
        WebServer_NotifyClients();
    }
}
