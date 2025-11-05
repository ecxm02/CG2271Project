#include "webServer.h"
#include <WiFi.h>
#include <WebServer.h>

#define AP_SSID "PlantWatering_ESP32"
#define AP_PASSWORD "12345678"

WebServer server(80);
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
    </style>
    <script>
        function sendCommand(cmd) {
            fetch('/cmd?c=' + cmd)
                .then(() => setTimeout(() => location.reload(), 500));
        }
        setInterval(() => location.reload(), 5000);
    </script>
</head>
<body>
    <div class="container">
        <h1>🌱 Plant Watering System</h1>
        
        <div class="sensor-card">
            <div class="sensor-name">Soil Moisture</div>
            <div class="sensor-value">%SOIL%&#37;</div>
        </div>
        
        <div class="sensor-card">
            <div class="sensor-name">Light Level</div>
            <div class="sensor-value">%LIGHT%&#37;</div>
        </div>
        
        <div class="sensor-card">
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
        
        <div class="controls">
            <h3>Manual Controls</h3>
            <button class="button" onclick="sendCommand('P1')">Pump ON</button>
            <button class="button button-danger" onclick="sendCommand('P0')">Pump OFF</button>
            <button class="button" onclick="sendCommand('L1')">LED ON</button>
            <button class="button button-danger" onclick="sendCommand('L0')">LED OFF</button>
        </div>
    </div>
</body>
</html>
)rawliteral";

void handleRoot() {
    String html = FPSTR(HTML_PAGE);
    
    html.replace("%SOIL%", String(currentStatus.soilMoisture));
    html.replace("%LIGHT%", String(currentStatus.lightLevel));
    html.replace("%WATER%", String(currentStatus.waterLevel));
    
    html.replace("%PUMP_STATUS%", currentStatus.pumpStatus ? "ON" : "OFF");
    html.replace("%PUMP_CLASS%", currentStatus.pumpStatus ? "status-on" : "status-off");
    
    html.replace("%LED_STATUS%", currentStatus.ledStatus ? "ON" : "OFF");
    html.replace("%LED_CLASS%", currentStatus.ledStatus ? "status-on" : "status-off");
    
    server.send(200, "text/html", html);
}

void handleCommand() {
    if (server.hasArg("c")) {
        String cmd = server.arg("c");
        Serial.println(cmd);
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void WebServer_Init(void) {
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    Serial.print("Access Point Started: ");
    Serial.println(AP_SSID);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    
    server.on("/", handleRoot);
    server.on("/cmd", handleCommand);
    
    server.begin();
    Serial.println("Web server started");
}

void WebServer_HandleClient(void) {
    server.handleClient();
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
