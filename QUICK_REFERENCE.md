# Quick Reference Guide - Plant Watering System

## 🔌 Hardware Setup
```
MCXC444 Connections:
├─ PTB0  → Soil Moisture Sensor (Digital)
├─ PTE30 → Light Sensor/LDR (Digital)
├─ PTA4  → Red LED (Low Water Alarm)
├─ PTD4  → Green LED (Pump Active)
├─ PTA5  → Blue LED (Dark/Night Indicator)
├─ PTD2  → Water Pump (Relay)
├─ PTD3  → Buzzer (Low Water Alarm) ⚠️ NEW
├─ PTA1  → UART RX (from ESP32 TX)
└─ PTA2  → UART TX (to ESP32 RX)

ESP32 Connections:
├─ GPIO34 → Water Level Sensor (Analog ADC)
├─ GPIO16 → UART RX (from MCXC444 TX)
└─ GPIO17 → UART TX (to MCXC444 RX)
```

## 📡 Communication Protocol

### MCXC444 → ESP32 (1 second interval)
```json
{"soil":1,"light":0,"water":75,"pump":1,"led":0}
```

### ESP32 → MCXC444 (1 second interval)
```
W75    // Water level update
```

### Web Interface → MCXC444 (Button clicks)
```
P1  → Pump ON
P0  → Pump OFF
L1  → LED ON
L0  → LED OFF
A1  → Auto Mode
```

## 🎮 Control Logic

### Auto Mode (Default)
| Condition | Action |
|-----------|--------|
| Soil dry (1) + Water > 20% | Pump ON + Green LED ON |
| Soil wet (0) or Water ≤ 20% | Pump OFF + Green LED OFF |
| Dark (1) | Blue LED ON |
| Bright (0) | Blue LED OFF |
| **Water < 20%** | **Buzzer ON + Red LED ON** ⚠️ |

### Manual Mode
- Activated by any control button (P1/P0/L1/L0)
- Override automatic behavior
- Click **Auto Mode** button to return

## 🌐 Web Interface Access
1. Connect to WiFi: **PlantWatering_ESP32**
2. Password: **12345678**
3. Open browser: **http://192.168.4.1**
4. Auto-refresh: Every 5 seconds

## 🔍 Sensor Display Interpretation
| Sensor | Digital Value | Display |
|--------|---------------|---------|
| Soil Moisture | 1 (dry) | 0% |
| Soil Moisture | 0 (wet) | 100% |
| Light Level | 1 (dark) | 0% |
| Light Level | 0 (bright) | 100% |
| Water Level | ADC reading | 0-100% (actual) |

**Note:** When water < 20%, the water level card turns RED and a pulsing warning banner appears!

## ⚡ Quick Commands (PowerShell)

### Navigate to ESP32 folder
```powershell
cd "c:\Users\famil\OneDrive\Documents\School\Sem 5\CG2271\Final Project\ESP32"
```

### Build and upload ESP32
```powershell
pio run --target upload ; pio device monitor
```

### Navigate to MCXC444 folder
```powershell
cd "c:\Users\famil\OneDrive\Documents\School\Sem 5\CG2271\Final Project"
```

## 🐛 Debug Output

### ESP32 Serial Monitor (115200 baud)
```
=== ESP32 Plant Watering System ===
Connect to WiFi: PlantWatering_ESP32
Password: 12345678
IP Address: 192.168.4.1
====================================

Sent water level to MCXC444: W75
Status Update - Soil:0% Light:100% Water:75% Pump:ON LED:OFF
Sending command to MCXC444: P1
MCXC444: Pump ON (Manual)
```

### MCXC444 UART Output (9600 baud)
```
FreeRTOS Plant Watering System Started
{"soil":1,"light":0,"water":75,"pump":1,"led":0}
Pump ON (Manual)
Auto Mode Enabled
SAFETY: Pump auto-off (timeout)
```

## 🚨 Alarms & Safety

| Alert | Trigger | Indication |
|-------|---------|------------|
| Low Water | Water < 20% | Buzzer ON + Red LED ON + Web Warning Banner |
| Pump Timeout | Running > 5s | Auto shutoff + UART message |
| Stack Overflow | Task error | UART error message + halt |

**Low Water Behavior:**
- Motor/Pump will NOT activate even if soil is dry (safety)
- Red pulsing warning banner on web interface
- Buzzer continuously beeps until refilled
- Red LED stays ON
- User must refill water reservoir to resume automatic watering

## 📝 Files Modified (Summary)
1. ✅ `protocol.h` - Added water level & auto mode commands
2. ✅ `uart_comm.c` - Parse W<value> and A1 commands
3. ✅ `main.c` - Buzzer control + water level handling
4. ✅ `buzzer.h/c` - NEW buzzer module
5. ✅ `webServer.cpp` - Fixed command forwarding + auto button
6. ✅ `main.cpp` (ESP32) - Water level transmission + interpretation
7. ✅ `config.h` - Buzzer pin definition

## ⚙️ Build Configuration

Make sure `buzzer.c` is included in your MCXC444 build system:
- MCUXpresso: Add to source folder
- Makefile: Add `buzzer.o` to object list
- CMake: Add to `add_executable()` or `target_sources()`

## 🧪 Testing Sequence
1. [ ] Power on MCXC444 → RGB LED test sequence
2. [ ] Power on ESP32 → Check WiFi AP created
3. [ ] Connect phone/laptop to WiFi
4. [ ] Open web interface → Verify sensor readings
5. [ ] Test manual pump control
6. [ ] Test manual LED control
7. [ ] Click Auto Mode → System returns to auto
8. [ ] Simulate low water → Check buzzer + red LED
9. [ ] Simulate dry soil → Check pump activation
10. [ ] Cover light sensor → Check blue LED

---
**Last Updated**: November 5, 2025
**Status**: ✅ Ready for deployment
