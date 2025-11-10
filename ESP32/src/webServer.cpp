#include "webServer.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#define AP_SSID "ESP32_Water_Level"
#define AP_PASSWORD "12345678"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
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
            <button class="button button-auto" onclick="setPumpAuto()">Set Pump AUTO</button>
            <button class="button button-auto" onclick="setLightAuto()">Set Light AUTO</button>
        </div>
    </div>

    <script>
        var ws = new WebSocket('ws://' + window.location.hostname + '/ws');
        
        ws.onopen = function() {
            console.log('WebSocket connected');
        };
        
        ws.onmessage = function(event) {
            const data = JSON.parse(event.data);
            
            document.getElementById('relayStatus').innerText = data.pumpStatus ? 'Relay ON' : 'Relay OFF';
            document.getElementById('lightStatus').innerText = data.ledStatus ? 'Light ON' : 'Light OFF';
            document.getElementById('waterLevel').innerText = 'Water Level: ' + data.waterLevel;
            
            const pumpMode = data.pumpMode ? 'MANUAL' : 'AUTO';
            const lightMode = data.lightMode ? 'MANUAL' : 'AUTO';
            
            document.getElementById('pumpMode').innerHTML = 
                '<span class="mode-indicator mode-' + pumpMode.toLowerCase() + '">' + pumpMode + '</span>';
            document.getElementById('lightMode').innerHTML = 
                '<span class="mode-indicator mode-' + lightMode.toLowerCase() + '">' + lightMode + '</span>';
        };
        
        ws.onerror = function(error) {
            console.error('WebSocket error:', error);
        };
        
        ws.onclose = function() {
            console.log('WebSocket disconnected');
        };

        function togglePump() {
            fetch('/togglePump')
                .then(response => response.text())
                .then(data => {
                    console.log('Pump toggled:', data);
                })
                .catch(error => console.error('Error:', error));
        }

        function toggleLight() {
            fetch('/toggleLight')
                .then(response => response.text())
                .then(data => {
                    console.log('Light toggled:', data);
                })
                .catch(error => console.error('Error:', error));
        }

        function setPumpAuto() {
            fetch('/setPumpAuto')
                .then(response => response.text())
                .then(data => {
                    console.log('Pump set to AUTO:', data);
                })
                .catch(error => console.error('Error:', error));
        }

        function setLightAuto() {
            fetch('/setLightAuto')
                .then(response => response.text())
                .then(data => {
                    console.log('Light set to AUTO:', data);
                })
                .catch(error => console.error('Error:', error));
        }
    </script>
</body>
</html>
)rawliteral";

void handleRoot(AsyncWebServerRequest *request) {
    request->send(200, "text/html", HTML_PAGE);
}

void handleStatus(AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"waterLevel\":" + String(currentStatus.waterLevel) + ",";
    json += "\"pumpStatus\":" + String(currentStatus.pumpStatus) + ",";
    json += "\"pumpMode\":" + String(currentStatus.pumpMode) + ",";
    json += "\"ledStatus\":" + String(currentStatus.ledStatus) + ",";
    json += "\"lightMode\":" + String(currentStatus.lightMode);
    json += "}";
    request->send(200, "application/json", json);
}

void handleTogglePump(AsyncWebServerRequest *request) {
    if (currentStatus.pumpStatus == 1) {
        Serial.println("TX -> PUMP_OFF");
        Serial1.println("PUMP_OFF");
        request->send(200, "text/plain", "Pump turning OFF...");
    } else {
        Serial.println("TX -> PUMP_ON");
        Serial1.println("PUMP_ON");
        request->send(200, "text/plain", "Pump turning ON...");
    }
}

void handleToggleLight(AsyncWebServerRequest *request) {
    if (currentStatus.ledStatus == 1) {
        Serial.println("TX -> LIGHT_OFF");
        Serial1.println("LIGHT_OFF");
        request->send(200, "text/plain", "Light turning OFF...");
    } else {
        Serial.println("TX -> LIGHT_ON");
        Serial1.println("LIGHT_ON");
        request->send(200, "text/plain", "Light turning ON...");
    }
}

void handleRelayLightStatus(AsyncWebServerRequest *request) {
    String relayStatus = currentStatus.pumpStatus ? "Relay ON" : "Relay OFF";
    String lightStatus = currentStatus.ledStatus ? "Light ON" : "Light OFF";
    String pumpMode = currentStatus.pumpMode ? "MANUAL" : "AUTO";
    String lightMode = currentStatus.lightMode ? "MANUAL" : "AUTO";
    String status = relayStatus + ", " + lightStatus + ", " + pumpMode + ", " + lightMode;
    
    Serial.printf("Webpage poll -> P:%d(%d) L:%d(%d)\n", 
                  currentStatus.pumpStatus, currentStatus.pumpMode,
                  currentStatus.ledStatus, currentStatus.lightMode);
    
    request->send(200, "text/plain", status);
}

void handleSetPumpAuto(AsyncWebServerRequest *request) {
    Serial.println("TX -> PUMP_AUTO");
    Serial1.println("PUMP_AUTO");
    request->send(200, "text/plain", "Pump set to AUTO");
}

void handleSetLightAuto(AsyncWebServerRequest *request) {
    Serial.println("TX -> LIGHT_AUTO");
    Serial1.println("LIGHT_AUTO");
    request->send(200, "text/plain", "Light set to AUTO");
}

void WebServer_Init(void) {
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    Serial.print("Access Point Started: ");
    Serial.println(AP_SSID);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    
    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, 
                  AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            Serial.println("WebSocket client connected");
        } else if (type == WS_EVT_DISCONNECT) {
            Serial.println("WebSocket client disconnected");
        }
    });
    
    server.addHandler(&ws);
    server.on("/", HTTP_GET, handleRoot);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/togglePump", HTTP_GET, handleTogglePump);
    server.on("/toggleLight", HTTP_GET, handleToggleLight);
    server.on("/relayLightStatus", HTTP_GET, handleRelayLightStatus);
    server.on("/setPumpAuto", HTTP_GET, handleSetPumpAuto);
    server.on("/setLightAuto", HTTP_GET, handleSetLightAuto);
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(404);
    });
    
    server.begin();
    Serial.println("Web server started");
}

void WebServer_HandleClient(void) {
    ws.cleanupClients();
}

void WebServer_NotifyClients(void) {
    String json = "{";
    json += "\"pumpStatus\":" + String(currentStatus.pumpStatus) + ",";
    json += "\"pumpMode\":" + String(currentStatus.pumpMode) + ",";
    json += "\"ledStatus\":" + String(currentStatus.ledStatus) + ",";
    json += "\"lightMode\":" + String(currentStatus.lightMode) + ",";
    json += "\"waterLevel\":" + String(currentStatus.waterLevel);
    json += "}";
    ws.textAll(json);
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
