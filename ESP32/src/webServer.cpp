#include "webServer.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#define AP_SSID "ESP32_Water_Level"
#define AP_PASSWORD "12345678"

AsyncWebServer server(80);
SystemStatus_t currentStatus = {0};

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Water Level Monitor</title>
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
        .status {
            background-color: #e8f5e9;
            border-left: 4px solid #4caf50;
            padding: 15px;
            margin: 15px 0;
            border-radius: 5px;
        }
        .status-value {
            font-size: 24px;
            color: #2e7d32;
            margin: 5px 0;
        }
        .mode-indicator {
            display: inline-block;
            padding: 4px 12px;
            border-radius: 12px;
            font-size: 14px;
            font-weight: bold;
            margin-left: 10px;
        }
        .mode-auto {
            background-color: #4caf50;
            color: white;
        }
        .mode-manual {
            background-color: #ff9800;
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
        .button:active {
            background-color: #084d8a;
        }
        .button-auto {
            background-color: #4caf50;
            width: calc(50% - 14px);
        }
        .button-auto:hover {
            background-color: #45a049;
        }
        .controls {
            text-align: center;
            margin-top: 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Water Level Monitor</h1>
        
        <div class="status">
            <div>Water Level (ADC Raw):</div>
            <div class="status-value" id="waterLevel">Loading...</div>
        </div>
        
        <div class="status">
            <div>Relay Status:</div>
            <div class="status-value" id="relayStatus">Loading...</div>
            <div id="pumpMode"></div>
        </div>
        
        <div class="status">
            <div>Light Status:</div>
            <div class="status-value" id="lightStatus">Loading...</div>
            <div id="lightMode"></div>
        </div>
        
        <div class="controls">
            <button class="button" onclick="togglePump()">Toggle Pump</button>
            <button class="button" onclick="toggleLight()">Toggle Light</button>
        </div>
        
        <div class="controls">
            <button class="button button-auto" onclick="setPumpAuto()">Pump → AUTO</button>
            <button class="button button-auto" onclick="setLightAuto()">Light → AUTO</button>
        </div>
    </div>

    <script>
        function updateStatus() {
            fetch('/status')
                .then(response => response.text())
                .then(data => {
                    document.getElementById('waterLevel').innerText = data;
                })
                .catch(error => console.error('Error:', error));
            
            fetch('/relayLightStatus')
                .then(response => response.text())
                .then(data => {
                    const parts = data.split(',');
                    if (parts.length >= 4) {
                        document.getElementById('relayStatus').innerText = parts[0].trim();
                        document.getElementById('lightStatus').innerText = parts[1].trim();
                        
                        const pumpMode = parts[2].trim();
                        const lightMode = parts[3].trim();
                        
                        document.getElementById('pumpMode').innerHTML = 
                            '<span class="mode-indicator mode-' + pumpMode.toLowerCase() + '">' + pumpMode + '</span>';
                        document.getElementById('lightMode').innerHTML = 
                            '<span class="mode-indicator mode-' + lightMode.toLowerCase() + '">' + lightMode + '</span>';
                    }
                })
                .catch(error => console.error('Error:', error));
        }

        function togglePump() {
            fetch('/togglePump')
                .then(response => response.text())
                .then(data => {
                    console.log('Pump toggled:', data);
                    updateStatus();
                })
                .catch(error => console.error('Error:', error));
        }

        function toggleLight() {
            fetch('/toggleLight')
                .then(response => response.text())
                .then(data => {
                    console.log('Light toggled:', data);
                    updateStatus();
                })
                .catch(error => console.error('Error:', error));
        }

        function setPumpAuto() {
            fetch('/setPumpAuto')
                .then(response => response.text())
                .then(data => {
                    console.log('Pump set to AUTO:', data);
                    updateStatus();
                })
                .catch(error => console.error('Error:', error));
        }

        function setLightAuto() {
            fetch('/setLightAuto')
                .then(response => response.text())
                .then(data => {
                    console.log('Light set to AUTO:', data);
                    updateStatus();
                })
                .catch(error => console.error('Error:', error));
        }

        setInterval(updateStatus, 1000);
        updateStatus();
    </script>
</body>
</html>
)rawliteral";

void handleRoot(AsyncWebServerRequest *request) {
    request->send(200, "text/html", HTML_PAGE);
}

void handleStatus(AsyncWebServerRequest *request) {
    String status = "Water Level: " + String(currentStatus.waterLevel);
    request->send(200, "text/plain", status);
}

void handleTogglePump(AsyncWebServerRequest *request) {
    if (currentStatus.pumpStatus == 1) {
        Serial1.println("PUMP_OFF");
        currentStatus.pumpStatus = 0;
        request->send(200, "text/plain", "Pump turned OFF");
    } else {
        Serial1.println("PUMP_ON");
        currentStatus.pumpStatus = 1;
        request->send(200, "text/plain", "Pump turned ON");
    }
}

void handleToggleLight(AsyncWebServerRequest *request) {
    if (currentStatus.ledStatus == 1) {
        Serial1.println("LIGHT_OFF");
        currentStatus.ledStatus = 0;
        request->send(200, "text/plain", "Light turned OFF");
    } else {
        Serial1.println("LIGHT_ON");
        currentStatus.ledStatus = 1;
        request->send(200, "text/plain", "Light turned ON");
    }
}

void handleRelayLightStatus(AsyncWebServerRequest *request) {
    String relayStatus = currentStatus.pumpStatus ? "Relay ON" : "Relay OFF";
    String lightStatus = currentStatus.ledStatus ? "Light ON" : "Light OFF";
    String pumpMode = currentStatus.pumpMode ? "MANUAL" : "AUTO";
    String lightMode = currentStatus.lightMode ? "MANUAL" : "AUTO";
    String status = relayStatus + ", " + lightStatus + ", " + pumpMode + ", " + lightMode;
    request->send(200, "text/plain", status);
}

void handleSetPumpAuto(AsyncWebServerRequest *request) {
    Serial1.println("PUMP_AUTO");
    currentStatus.pumpMode = 0;
    request->send(200, "text/plain", "Pump set to AUTO");
}

void handleSetLightAuto(AsyncWebServerRequest *request) {
    Serial1.println("LIGHT_AUTO");
    currentStatus.lightMode = 0;
    request->send(200, "text/plain", "Light set to AUTO");
}

void WebServer_Init(void) {
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    Serial.print("Access Point Started: ");
    Serial.println(AP_SSID);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    
    server.on("/", HTTP_GET, handleRoot);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/togglePump", HTTP_GET, handleTogglePump);
    server.on("/toggleLight", HTTP_GET, handleToggleLight);
    server.on("/relayLightStatus", HTTP_GET, handleRelayLightStatus);
    server.on("/setPumpAuto", HTTP_GET, handleSetPumpAuto);
    server.on("/setLightAuto", HTTP_GET, handleSetLightAuto);
    
    server.begin();
    Serial.println("Web server started");
}

void WebServer_HandleClient(void) {
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
