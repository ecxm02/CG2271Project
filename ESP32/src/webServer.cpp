#include "webServer.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "../Common/config.h"

#define AP_SSID "PlantWatering_ESP32"
#define AP_PASSWORD "12345678"

AsyncWebServer server(80);
static SystemStatus_t currentStatus = {0};

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Plant Watering System</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 600px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f0f0f0;
        }
        .container {
            background-color: white;
            border-radius: 10px;
            padding: 20px;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
        }
        h1 {
            color: #2e7d32;
            text-align: center;
        }
        .sensor-card {
            background-color: #e8f5e9;
            border-left: 4px solid #4caf50;
            padding: 15px;
            margin: 10px 0;
            border-radius: 5px;
        }
        .sensor-name {
            font-weight: bold;
            color: #1b5e20;
        }
        .sensor-value {
            font-size: 24px;
            color: #2e7d32;
            margin: 5px 0;
        }
        .status {
            display: inline-block;
            padding: 5px 15px;
            border-radius: 15px;
            font-weight: bold;
            margin: 5px 0;
        }
        .status-on {
            background-color: #4caf50;
            color: white;
        }
        .status-off {
            background-color: #f44336;
            color: white;
        }
        .button {
            background-color: #2196F3;
            border: none;
            color: white;
            padding: 12px 24px;
            text-align: center;
            font-size: 16px;
            margin: 5px;
            cursor: pointer;
            border-radius: 5px;
            width: calc(50% - 14px);
        }
        .button:hover {
            background-color: #0b7dda;
        }
        .button-danger {
            background-color: #f44336;
        }
        .button-danger:hover {
            background-color: #da190b;
        }
        .controls {
            text-align: center;
            margin-top: 20px;
        }
        .warning-banner {
            background-color: #f44336;
            color: white;
            padding: 20px;
            margin: 15px 0;
            border-radius: 10px;
            text-align: center;
            font-weight: bold;
            font-size: 18px;
            animation: pulse 2s infinite;
            box-shadow: 0 4px 10px rgba(244, 67, 54, 0.4);
        }
        .warning-banner .icon {
            font-size: 40px;
            margin-bottom: 10px;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.7; }
        }
        .water-low {
            background-color: #ffebee;
            border-left: 4px solid #f44336;
        }
        .water-low .sensor-value {
            color: #c62828;
        }
    </style>
    <script>
        setInterval(() => location.reload(), 5000);
    </script>
</head>
<body>
    <div class="container">
        <h1>🌱 Plant Watering System</h1>
        
        %WARNING_BANNER%
        
        <div class="sensor-card">
            <div class="sensor-name">Soil Moisture</div>
            <div class="sensor-value">%SOIL%&#37;</div>
        </div>
        
        <div class="sensor-card">
            <div class="sensor-name">Light Level</div>
            <div class="sensor-value">%LIGHT%&#37;</div>
        </div>
        
        <div class="sensor-card %WATER_CLASS%">
            <div class="sensor-name">Water Level</div>
            <div class="sensor-value">%WATER%&#37;</div>
        </div>
        
        <div class="sensor-card">
            <div class="sensor-name">Water Pump</div>
            <div class="status %PUMP_CLASS%">%PUMP_STATUS%</div>
        </div>
        
        <div class="sensor-card">
            <div class="sensor-name">LED Light</div>
            <div class="status %LED_CLASS%">%LED_STATUS%</div>
        </div>
        
    </div>
</body>
</html>
)rawliteral";

String processor(const String& var){
    if(var == "SOIL"){
        return String(currentStatus.soilMoisture);
    }
    else if(var == "LIGHT"){
        return String(currentStatus.lightLevel);
    }
    else if(var == "WATER"){
        return String(currentStatus.waterLevel);
    }
    else if(var == "PUMP_STATUS"){
        return currentStatus.pumpStatus ? "ON" : "OFF";
    }
    else if(var == "PUMP_CLASS"){
        return currentStatus.pumpStatus ? "status-on" : "status-off";
    }
    else if(var == "LED_STATUS"){
        return currentStatus.ledStatus ? "ON" : "OFF";
    }
    else if(var == "LED_CLASS"){
        return currentStatus.ledStatus ? "status-on" : "status-off";
    }
    else if(var == "WARNING_BANNER"){
        if (currentStatus.waterLevel < WATER_LOW_THRESHOLD) {
            String warningBanner = "<div class=\"warning-banner\">";
            warningBanner += "<div class=\"icon\">⚠️</div>";
            warningBanner += "<div>CRITICAL: WATER LEVEL TOO LOW!</div>";
            warningBanner += "<div style=\"font-size: 14px; margin-top: 10px;\">Please refill water reservoir immediately</div>";
            warningBanner += "<div style=\"font-size: 14px;\">🔴 Buzzer Active | 🚫 Watering Disabled</div>";
            warningBanner += "</div>";
            return warningBanner;
        }
        return "";
    }
    else if(var == "WATER_CLASS"){
        return (currentStatus.waterLevel < WATER_LOW_THRESHOLD) ? "water-low" : "";
    }
    return String();
}

void WebServer_Init(void) {
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    Serial.print("Access Point Started: ");
    Serial.println(AP_SSID);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", HTML_PAGE, processor);
    });

    server.on("/command", HTTP_GET, [] (AsyncWebServerRequest *request) {
        if (request->hasParam("c")) {
            String cmd = request->getParam("c")->value();
            Serial.print("Sending command to MCXC444: ");
            Serial.println(cmd);
            Serial1.println(cmd);
            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Bad Request");
        }
    });
    
    server.begin();
    Serial.println("Web server started");
}

void WebServer_HandleClient(void) {
    // No longer needed with ESPAsyncWebServer
}

void WebServer_UpdateStatus(SystemStatus_t *status) {
    if (status) {
        currentStatus = *status;
    }
}

const char* WebServer_GetSSID(void) {
    return AP_SSID;
}

const char* WebServer_GetPassword(void) {
    return AP_PASSWORD;
}

const char* WebServer_GetIPAddress(void) {
    static char ipStr[16];
    IPAddress ip = WiFi.softAPIP();
    snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return ipStr;
}
