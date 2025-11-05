# System Changes Summary

## Date: November 5, 2025

### Architecture Overview
The system now properly implements bidirectional communication between ESP32 and MCXC444:
- **MCXC444** → **ESP32**: Sends digital sensor data (soil moisture, light level) + actuator status
- **ESP32** → **MCXC444**: Sends water level data + user commands from web interface
- **ESP32**: Interprets digital sensor data and displays meaningful values on web interface

---

## Changes Made

### 1. Protocol Updates (`Common/protocol.h`)
**Added new commands:**
- `CMD_SET_WATER_LEVEL` (7): ESP32 sends water level to MCXC444
- `CMD_AUTO_MODE` (8): Switch from manual back to automatic mode
- `CMD_WATER_LEVEL_STR "W"`: Protocol string for water level
- `CMD_AUTO_MODE_STR "A1"`: Protocol string for auto mode

---

### 2. MCXC444 UART Communication (`MCXC444/source/uart_comm.c`)
**Enhanced command parsing:**
- Added parsing for `W<value>` (water level) - extracts numeric value from command
- Added parsing for `A1` (auto mode switch)
- Water level parsing converts ASCII digits to integer (e.g., "W75" → 75)

---

### 3. MCXC444 Main Logic (`MCXC444/source/main.c`)

**Added buzzer support:**
- Included `buzzer.h` header
- Initialized buzzer in `main()`

**Updated UARTTask:**
- Handles `CMD_SET_WATER_LEVEL`: Updates global `waterLevel` variable
- Handles `CMD_AUTO_MODE`: Switches system back to automatic mode
- Both commands received from ESP32 via UART

**Updated ControlTask:**
- **Low water alarm**: 
  - If water level < 20%, buzzer ON + red LED ON
  - If water level >= 20%, buzzer OFF + red LED OFF
- Automatic watering logic remains unchanged
- Blue LED for dark conditions (light sensor)
- Green LED indicates pump active

---

### 4. Buzzer Module (NEW)
**Created files:**
- `MCXC444/includes/buzzer.h`
- `MCXC444/source/buzzer.c`

**Hardware:**
- Port: PORTD, Pin: PTD3
- Functions: `Buzzer_Init()`, `Buzzer_On()`, `Buzzer_Off()`, `Buzzer_Toggle()`

---

### 5. ESP32 Web Server (`ESP32/webServer.cpp`)

**Fixed command forwarding:**
```cpp
// OLD: Serial.println(cmd);  // Only printed to serial monitor
// NEW: Serial2.println(cmd);  // Sends to MCXC444 via UART
```

**Added Auto Mode button:**
- Green button at bottom of controls
- Sends "A1" command to MCXC444
- Returns system to automatic mode after manual override

**Added Low Water Warning Banner:**
- Prominent red pulsing warning banner appears when water < 20%
- Shows critical alert message
- Indicates buzzer is active and watering is disabled
- Water level card changes to red theme when low
- Visual feedback helps user understand system state

---

### 6. ESP32 Main Loop (`ESP32/main.cpp`)

**Water level transmission:**
- Every 1 second, ESP32 sends `W<value>` to MCXC444
- Example: If water level is 75%, sends "W75\n"
- MCXC444 receives this and updates its internal water level variable

**Digital sensor interpretation:**
```cpp
// Soil Moisture: 1 (dry) → 0%, 0 (wet) → 100%
systemStatus.soilMoisture = soilDigital ? 0 : 100;

// Light Level: 1 (dark) → 0%, 0 (bright) → 100%  
systemStatus.lightLevel = lightDigital ? 0 : 100;
```

---

### 7. Configuration (`Common/config.h`)
**Added buzzer pin definition:**
```c
#define BUZZER_PORT PORTD
#define BUZZER_PIN 3
```

---

## Communication Protocol

### MCXC444 → ESP32 (Every 1 second)
```json
{"soil":0,"light":1,"water":75,"pump":1,"led":0}
```
- `soil`: 0 or 1 (digital sensor - 1=dry, 0=wet)
- `light`: 0 or 1 (digital sensor - 1=dark, 0=bright)
- `water`: 0-100 (percentage from previous ESP32 update)
- `pump`: 0 or 1 (pump status)
- `led`: 0 or 1 (blue LED status)

### ESP32 → MCXC444 (Every 1 second)
```
W75
```
- Format: `W<0-100>`
- Updates MCXC444's water level for buzzer triggering

### ESP32 → MCXC444 (On button press)
```
P1   - Pump ON
P0   - Pump OFF
L1   - LED ON
L0   - LED OFF
A1   - Auto Mode
```

### MCXC444 → ESP32 (Acknowledgments)
```
Pump ON (Manual)
Pump OFF (Manual)
LED ON (Manual)
LED OFF (Manual)
Auto Mode Enabled
```

---

## System Behavior

### Automatic Mode
1. **Soil is dry (sensor=1) AND water > 20%** → Motor/Pump ON, Green LED ON ✅ Water released
2. **Soil is dry (sensor=1) BUT water ≤ 20%** → Motor/Pump OFF, Buzzer ON, Red LED ON ⚠️ Cannot water - out of reserve
3. **Soil is wet (sensor=0)** → Motor/Pump OFF, Green LED OFF
4. **Dark (sensor=1)** → Blue LED ON (nighttime indicator)
5. **Bright (sensor=0)** → Blue LED OFF
6. **Water < 20%** → Buzzer ON, Red LED ON, Web warning banner displayed 🚨

### Manual Mode
- User clicks buttons on web interface
- Commands sent to MCXC444
- Manual override takes effect
- Click "Auto Mode" button to return to automatic control

### Safety Features
1. **Pump timeout**: Auto-off after 5 seconds (SafetyTask) - prevents motor damage
2. **Low water protection**: Won't activate motor/pump if water < 20% - prevents dry running
3. **Buzzer alarm**: Alerts when water level critically low - user knows to refill
4. **Web warning**: Red pulsing banner shows critical water status
5. **Independent operation**: MCXC444 continues to function even if ESP32 fails

---

## Web Interface Display

### Sensor Interpretations (ESP32 side)
- **Soil Moisture**: 
  - Digital 0 (wet) → Display "100%"
  - Digital 1 (dry) → Display "0%"
  
- **Light Level**:
  - Digital 0 (bright) → Display "100%"
  - Digital 1 (dark) → Display "0%"
  
- **Water Level**: 
  - Actual ADC reading from ESP32 (0-100%)

### Control Buttons
- Pump ON / Pump OFF
- LED ON / LED OFF
- Auto Mode (returns to automatic control)
- Auto-refresh every 5 seconds

---

## Hardware Connections

| Component | MCXC444 Pin | Notes |
|-----------|-------------|-------|
| Soil Moisture Sensor | PTB0 | Digital input (dry=HIGH, wet=LOW) |
| Light Sensor (LDR) | PTE30 | Digital input (dark=HIGH, bright=LOW) |
| Red LED | PTA4 | Low water alarm indicator |
| Green LED | PTD4 | Pump active indicator |
| Blue LED | PTA5 | Nighttime/dark indicator |
| Water Pump | PTD2 | Relay control |
| **Buzzer** | **PTD3** | **Low water alarm** |
| UART TX | PTA2 | To ESP32 RX (GPIO16) |
| UART RX | PTA1 | From ESP32 TX (GPIO17) |

| Component | ESP32 Pin | Notes |
|-----------|-----------|-------|
| Water Level Sensor | GPIO34 (ADC) | Analog 0-100% |
| UART RX | GPIO16 | From MCXC444 TX |
| UART TX | GPIO17 | To MCXC444 RX |

---

## Testing Checklist

### ✅ UART Communication
- [ ] ESP32 receives sensor status from MCXC444 every 1s
- [ ] ESP32 sends water level to MCXC444 every 1s
- [ ] Web buttons send commands to MCXC444
- [ ] MCXC444 responds with acknowledgments

### ✅ Sensor Display
- [ ] Soil: Shows 0% when dry, 100% when wet
- [ ] Light: Shows 0% when dark, 100% when bright
- [ ] Water: Shows actual percentage from ADC

### ✅ Automatic Control
- [ ] Pump activates when soil dry AND water > 20%
- [ ] Blue LED turns on in dark conditions
- [ ] Buzzer + Red LED when water < 20%

### ✅ Manual Control
- [ ] Pump ON/OFF buttons work
- [ ] LED ON/OFF buttons work
- [ ] Auto Mode button returns to automatic

### ✅ Safety
- [ ] Pump auto-shutoff after 5 seconds
- [ ] Buzzer alerts when water low
- [ ] System stable with no crashes

---

## Files Modified

1. `Common/protocol.h` - Added new commands
2. `Common/config.h` - Added buzzer pin config
3. `MCXC444/source/uart_comm.c` - Enhanced command parsing
4. `MCXC444/source/main.c` - Added buzzer logic + water level handling
5. `MCXC444/includes/buzzer.h` - NEW buzzer header
6. `MCXC444/source/buzzer.c` - NEW buzzer implementation
7. `ESP32/webServer.cpp` - Fixed command forwarding + added auto mode button
8. `ESP32/main.cpp` - Added water level transmission + sensor interpretation

---

## Next Steps for Deployment

1. **Compile MCXC444 code** - Ensure buzzer.c is included in build
2. **Upload ESP32 code** - Flash via PlatformIO
3. **Connect hardware**:
   - Connect buzzer to PTD3
   - Connect UART cross-over (TX↔RX)
   - Connect sensors and actuators
4. **Power on** - MCXC444 first, then ESP32
5. **Connect to WiFi** - "PlantWatering_ESP32" / "12345678"
6. **Open browser** - http://192.168.4.1
7. **Test all functions** - Use checklist above

---

## Troubleshooting

**Issue**: Web interface shows 0% or 1% for sensors
- **Cause**: Digital sensors only return 0 or 1
- **Fix**: Already implemented - ESP32 converts to 0% or 100%

**Issue**: Buzzer not working
- **Check**: PTD3 pin connection, buzzer polarity
- **Debug**: Add UART_SendString() in ControlTask when buzzer activates

**Issue**: Commands don't reach MCXC444
- **Check**: Serial2.println() in webServer.cpp (not Serial.println())
- **Debug**: Monitor Serial output on ESP32 for sent commands

**Issue**: Water level not updating on MCXC444
- **Check**: "W<value>" being sent every 1 second from ESP32
- **Debug**: Add UART_SendString() when water level received

---

**System Status**: ✅ All fixes implemented and ready for testing!
