# 🌱 Plant Watering System

An automated plant watering system using **MCXC444** (KL25Z) with **FreeRTOS** for real-time control and **ESP32** for WiFi web interface.

---

## 📐 System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32 (Web Server)                       │
│  • Water Level Sensor (ADC Pin 34)                          │
│  • WiFi Access Point + Web Dashboard                        │
│  • UART RX from MCXC444 (GPIO16)                            │
│  • UART TX to MCXC444 (GPIO17)                              │
└──────────────────┬──────────────────────────────────────────┘
                   │ UART @ 9600 baud
                   │ JSON Status + Commands
┌──────────────────▼──────────────────────────────────────────┐
│         MCXC444 + FreeRTOS (Smart Controller)               │
│  • 4 Real-Time Tasks (Sensor/Control/UART/Safety)           │
│  • Soil Moisture Sensor (ADC0_SE8 / PTB0)                   │
│  • LDR Light Sensor (ADC0_SE9 / PTB1)                       │
│  • LED Control (PTD1)                                       │
│  • Water Pump Control (PTD2)                                │
│  • UART to ESP32 (PTA1=RX, PTA2=TX)                         │
│  • AUTO-DECISION MAKING ✅                                  │
│  • Interrupt-Driven UART RX ⚡                              │
└─────────────────────────────────────────────────────────────┘
```

### **Design Philosophy: MCXC444 Does the Thinking**

✅ **MCXC444 (Main Controller with FreeRTOS)**
- **4 concurrent tasks** for real-time operation
- Reads all sensors locally (100ms cycle)
- Makes autonomous watering decisions (priority-based)
- Controls LED based on light levels
- Operates independently (no WiFi dependency)
- Sends status updates to ESP32 (1s cycle)
- **Interrupt-driven UART** for immediate command response
- **Priority-based safety monitoring** (pump timeout)

✅ **ESP32 (Web Interface)**
- Displays real-time system status
- Allows manual override controls
- Monitors water level in reservoir
- Forwards user commands to MCXC444

## 🗂️ Project Structure

```
CG2271Project/
├── MCXC444/                    # Main controller firmware (FreeRTOS)
│   ├── main.c                  # FreeRTOS tasks & control logic
│   ├── FreeRTOSConfig.h        # FreeRTOS configuration
│   ├── led.c / led.h           # LED driver
│   ├── soilMoisture.c / .h     # Soil moisture sensor (ADC)
│   ├── lightSensor.c / .h      # LDR light sensor (ADC)
│   ├── waterPump.c / .h        # Water pump control
│   └── uart_comm.c / .h        # UART communication (interrupt-driven)
│
├── ESP32/                      # Web server firmware
│   ├── main.cpp                # Main program
│   ├── waterLevel.c / .h       # Water level sensor
│   ├── webServer.cpp / .h      # WiFi web server
│   └── platformio.ini          # PlatformIO config
│
└── Common/                     # Shared definitions
    ├── protocol.h              # Communication protocol
    └── config.h                # System configuration
```

---

## ⚡ FreeRTOS Task Architecture

### **4 Real-Time Tasks:**

1. **SensorTask** (Priority 2, 100ms period)
   - Reads soil moisture, light level
   - Non-blocking ADC reads
   - Sends data to ControlTask via queue

2. **ControlTask** (Priority 3 - Highest)
   - Receives sensor data from queue
   - Makes watering decisions (auto mode)
   - Controls LED based on light level
   - Protected by mutexes

3. **UARTTask** (Priority 2)
   - Handles ESP32 communication
   - Processes manual commands (interrupt-driven RX)
   - Sends status updates every 1 second
   - Mode switching (auto/manual)

4. **SafetyTask** (Priority 4 - Critical)
   - Monitors pump runtime
   - Auto-shutoff after 5 seconds
   - Highest priority for safety

**See `FREERTOS_GUIDE.md` for detailed task information.**

---

## 🔌 Hardware Connections

### **MCXC444 (KL25Z) Pinout**

| Component | Pin | Type | Notes |
|-----------|-----|------|-------|
| Soil Moisture Sensor | PTB0 (ADC0_SE8) | Analog Input | 0-3.3V |
| LDR Light Sensor | PTB1 (ADC0_SE9) | Analog Input | Voltage divider circuit |
| LED | PTD1 | Digital Output | Active HIGH |
| Water Pump | PTD2 | Digital Output | Use relay/transistor |
| UART TX (to ESP32) | PTA2 | UART0_TX | Connect to ESP32 RX |
| UART RX (from ESP32) | PTA1 | UART0_RX | Connect to ESP32 TX |

### **ESP32 Pinout**

| Component | Pin | Type | Notes |
|-----------|-----|------|-------|
| Water Level Sensor | GPIO34 | Analog Input | ADC1_CH6 |
| UART TX (to MCXC444) | GPIO17 | UART2_TX | Connect to MCXC444 RX |
| UART RX (from MCXC444) | GPIO16 | UART2_RX | Connect to MCXC444 TX |

### **Wiring Diagram**

```
MCXC444          ESP32
PTA2 (TX) -----> GPIO16 (RX)
PTA1 (RX) <----- GPIO17 (TX)
GND       -----> GND
```

---

## ⚙️ System Logic (Auto Mode)

### **MCXC444 Control Flow**

```c
Every 100ms:
    1. Read soil moisture sensor
    2. Read light sensor
    3. Check for commands from ESP32
    
    AUTO MODE:
        IF light level < 30% THEN
            Turn LED ON
        ELSE
            Turn LED OFF
        
        IF soil moisture < 30% AND water level > 20% THEN
            Turn pump ON (max 5 seconds)
        ELSE
            Turn pump OFF
    
    MANUAL MODE (from web interface):
        Execute user commands directly
    
Every 1 second:
    Send status update to ESP32 via UART
```

---

## 📡 Communication Protocol

### **MCXC444 → ESP32 (Status Updates)**

Format: JSON string via UART
```json
{"soil":45,"light":78,"water":80,"pump":0,"led":1}
```

Fields:
- `soil`: Soil moisture percentage (0-100)
- `light`: Light level percentage (0-100)
- `water`: Water level percentage (0-100)
- `pump`: Pump status (0=OFF, 1=ON)
- `led`: LED status (0=OFF, 1=ON)

### **ESP32 → MCXC444 (Commands)**

Format: 2-character ASCII commands + newline

| Command | Description |
|---------|-------------|
| `P1\n` | Turn pump ON |
| `P0\n` | Turn pump OFF |
| `L1\n` | Turn LED ON |
| `L0\n` | Turn LED OFF |

---

## 🌐 Web Interface

### **Accessing the Dashboard**

1. **Power on ESP32**
2. **Connect to WiFi**
   - SSID: `PlantWatering_ESP32`
   - Password: `12345678`
3. **Open browser**
   - URL: `http://192.168.4.1`

### **Dashboard Features**

📊 **Real-time Monitoring**
- Soil moisture level (%)
- Light level (%)
- Water reservoir level (%)
- Pump status (ON/OFF)
- LED status (ON/OFF)

🎮 **Manual Controls**
- Force pump ON/OFF
- Force LED ON/OFF
- Auto-refresh every 5 seconds

---

## 🛠️ Configuration & Thresholds

All thresholds can be adjusted in `Common/config.h`:

```c
#define SOIL_DRY_THRESHOLD 30        // Soil < 30% = dry
#define LIGHT_DARK_THRESHOLD 30      // Light < 30% = dark
#define WATER_LOW_THRESHOLD 20       // Water < 20% = low
#define MAX_PUMP_DURATION_MS 5000    // Max pump runtime (safety)
```

### **Pin Configuration**

If you need to change pins, edit these in the respective files:

**MCXC444** (`led.c`, `waterPump.c`, etc.):
```c
#define LED_PIN 1           // Change LED pin
#define PUMP_PIN 2          // Change pump pin
#define SOIL_ADC_CHANNEL 8  // Change soil sensor ADC
#define LIGHT_ADC_CHANNEL 9 // Change light sensor ADC
```

**ESP32** (`waterLevel.c`):
```c
#define WATER_LEVEL_PIN 34  // Change water sensor pin
```

---

## 🚀 Getting Started

### **For MCXC444 (MCUXpresso IDE)**

1. Open MCUXpresso IDE
2. Import project from `MCXC444` folder
3. Build project
4. Flash to MCXC444 board
5. Connect sensors according to pinout table

### **For ESP32 (PlatformIO)**

1. Install PlatformIO in VS Code
2. Open `ESP32` folder as PlatformIO project
3. Build and upload:
   ```bash
   cd "C:\Users\famil\OneDrive\Documents\School\Sem 5\CG2271\Final Project\ESP32"
   pio run --target upload
   ```
4. Monitor serial output:
   ```bash
   pio device monitor
   ```

### **For ESP32 (Arduino IDE)**

1. Install ESP32 board support
2. Copy all files from `ESP32` to sketch folder
3. Rename `main.cpp` to `main.ino`
4. Select board: "ESP32 Dev Module"
5. Upload

---

## 🔍 Troubleshooting

### **Problem: MCXC444 not sending data**
- Check UART wiring (TX/RX crossed)
- Verify baud rate (9600)
- Check ground connection

### **Problem: ESP32 web page not loading**
- Ensure connected to ESP32 WiFi
- Try `http://192.168.4.1` directly
- Check ESP32 serial monitor for IP

### **Problem: Sensors reading 0 or max value**
- Verify sensor power (3.3V)
- Check ADC pin connections
- Ensure proper sensor grounding

### **Problem: Pump not turning on**
- Check relay/transistor circuit
- Verify sufficient power for pump
- Check GPIO pin configuration

---

## 📊 Sensor Calibration

### **Soil Moisture Sensor**
```c
// Dry soil → Low voltage → Low ADC reading
// Wet soil → High voltage → High ADC reading
// Adjust threshold in config.h based on testing
```

### **LDR Light Sensor**
```c
// Circuit: 3.3V ─── [10kΩ] ─── ADC_PIN ─── [LDR] ─── GND
// Bright light → Low resistance → High voltage
// Dark → High resistance → Low voltage
```

### **Water Level Sensor**
```c
// No water → Low voltage
// Full water → High voltage
// Test with known levels and adjust threshold
```

---

## 🎓 Learning Outcomes

This project demonstrates:
- ✅ Embedded C programming
- ✅ ADC for analog sensors
- ✅ GPIO for digital control
- ✅ UART serial communication
- ✅ Interrupt-driven I/O
- ✅ ESP32 WiFi web server
- ✅ Real-time control systems
- ✅ Modular code architecture

---

## 📝 To-Do / Future Enhancements

- [ ] Add data logging (SD card or cloud)
- [ ] Implement scheduled watering times
- [ ] Add temperature/humidity sensor
- [ ] Battery backup for MCXC444
- [ ] Mobile app interface
- [ ] Email/SMS notifications
- [ ] Multi-zone watering control

---

## 📄 License

This is a student project for CG2271 course.

---

## 👥 Contributors

- Your Name (Student ID)

---

## 📞 Support

For questions or issues:
1. Check the troubleshooting section
2. Review code comments
3. Contact course instructor

**Happy Coding! 🌱💧**
