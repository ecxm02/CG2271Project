# Water Level Logic Flow

## System Behavior Based on Conditions

```
┌─────────────────────────────────────────────────────────────────────┐
│                    WATER LEVEL DECISION TREE                        │
└─────────────────────────────────────────────────────────────────────┘

Start: System reads sensors every 100ms
    │
    ├─► Water Level Check (Happens ALWAYS)
    │   │
    │   ├─► Water ≥ 20% (SUFFICIENT)
    │   │   ├─ Buzzer: OFF
    │   │   ├─ Red LED: OFF
    │   │   └─ Web UI: Normal green theme
    │   │
    │   └─► Water < 20% (CRITICAL LOW) ⚠️
    │       ├─ Buzzer: ON (Continuous alarm)
    │       ├─ Red LED: ON
    │       └─ Web UI: Red pulsing warning banner
    │
    └─► Soil Moisture Check (AUTO MODE ONLY)
        │
        ├─► Soil is WET (sensor=0)
        │   ├─ Motor/Pump: OFF
        │   └─ Green LED: OFF
        │
        └─► Soil is DRY (sensor=1)
            │
            ├─► Water > 20% (Sufficient reserve)
            │   ├─ Motor/Pump: ON ✅ (Water released)
            │   ├─ Green LED: ON
            │   └─ Auto-shutoff after 5 seconds
            │
            └─► Water ≤ 20% (Insufficient reserve)
                ├─ Motor/Pump: OFF 🚫 (Safety prevention)
                ├─ Green LED: OFF
                ├─ Buzzer: ON (Alarm)
                ├─ Red LED: ON
                └─ Web UI: "Cannot water - refill reservoir"
```

## Real-World Scenarios

### Scenario 1: Normal Operation ✅
```
Water Level: 75%
Soil: Dry (sensor=1)
Light: Dark (sensor=1)

Actions:
├─ Motor/Pump: ON (releases water)
├─ Green LED: ON (pump active)
├─ Blue LED: ON (nighttime)
├─ Red LED: OFF
├─ Buzzer: OFF
└─ Web UI: Normal display, all green
```

### Scenario 2: Low Water Emergency 🚨
```
Water Level: 15%
Soil: Dry (sensor=1)
Light: Bright (sensor=0)

Actions:
├─ Motor/Pump: OFF (safety - cannot water)
├─ Green LED: OFF
├─ Blue LED: OFF
├─ Red LED: ON (alarm indicator)
├─ Buzzer: ON (continuous alarm)
└─ Web UI: RED PULSING WARNING BANNER
   "CRITICAL: WATER LEVEL TOO LOW!"
   "Please refill water reservoir immediately"
   "🔴 Buzzer Active | 🚫 Watering Disabled"
```

### Scenario 3: Water Refilled ✅
```
Water Level: 80% (just refilled)
Soil: Still Dry (sensor=1)
Light: Bright (sensor=0)

Actions:
├─ Motor/Pump: ON (now can water again)
├─ Green LED: ON
├─ Blue LED: OFF
├─ Red LED: OFF (alarm cleared)
├─ Buzzer: OFF (alarm cleared)
└─ Web UI: Warning banner disappears, normal green theme
```

### Scenario 4: Manual Override with Low Water ⚠️
```
Water Level: 10%
Soil: Dry (sensor=1)
User clicks: "Pump ON" button

Actions:
├─ System Mode: Switches to MANUAL
├─ Motor/Pump: ON (user override - use with caution!)
├─ Red LED: Still ON (water still low)
├─ Buzzer: Still ON (water still low)
├─ Web UI: Still shows warning banner
└─ Safety: Pump auto-shutoff after 5 seconds
```

## LED Color Meanings

| LED Color | Meaning | Trigger Condition |
|-----------|---------|-------------------|
| 🔴 Red | Low Water Alarm | Water < 20% |
| 🟢 Green | Motor/Pump Active | Pump is ON |
| 🔵 Blue | Nighttime/Dark | Light sensor detects darkness |

## Web Interface States

### Normal State (Water ≥ 20%)
```
┌───────────────────────────────────┐
│  🌱 Plant Watering System         │
├───────────────────────────────────┤
│  Soil Moisture: 0%  [DRY]         │
│  Light Level: 100%  [BRIGHT]      │
│  Water Level: 75%   [OK]          │
│  Pump: ON                         │
│  LED: OFF                         │
├───────────────────────────────────┤
│  Manual Controls:                 │
│  [Pump ON] [Pump OFF]             │
│  [LED ON]  [LED OFF]              │
│  [🔄 Auto Mode]                   │
└───────────────────────────────────┘
```

### Critical State (Water < 20%)
```
┌───────────────────────────────────┐
│  🌱 Plant Watering System         │
├───────────────────────────────────┤
│ ⚠️ PULSING RED BANNER ⚠️          │
│  CRITICAL: WATER LEVEL TOO LOW!   │
│  Please refill water immediately  │
│  🔴 Buzzer Active | 🚫 Disabled   │
├───────────────────────────────────┤
│  Soil Moisture: 0%  [DRY]         │
│  Light Level: 100%  [BRIGHT]      │
│  Water Level: 15%   [CRITICAL]    │
│    ↑ Red background, red text     │
│  Pump: OFF                        │
│  LED: OFF                         │
├───────────────────────────────────┤
│  Manual Controls:                 │
│  [Pump ON] [Pump OFF]             │
│  [LED ON]  [LED OFF]              │
│  [🔄 Auto Mode]                   │
└───────────────────────────────────┘
```

## UART Communication During Low Water

### ESP32 → MCXC444 (Every 1 second)
```
W15    // Sends low water level (15%)
```

### MCXC444 → ESP32 (Every 1 second)
```json
{"soil":1,"light":0,"water":15,"pump":0,"led":0}
       ↑ dry         ↑ bright  ↑ low   ↑ off
```

### ESP32 Processing
```cpp
if (waterLevel < 20) {
    // Show warning banner
    // Highlight water card in red
}
```

### MCXC444 Processing
```c
if (waterLevel < MIN_WATER_LEVEL) {
    Buzzer_On();           // Alert user
    LED_On(LED_RED);       // Visual indicator
    // Pump stays OFF even if soil is dry
}
```

## Hardware Response Timeline

```
Time: 0s
├─ Water level drops to 19%
├─ ESP32 sends: "W19\n"
└─ MCXC444 receives water level

Time: 0.1s
├─ MCXC444 ControlTask processes
├─ Detects: waterLevel < MIN_WATER_LEVEL (20)
├─ Buzzer: OFF → ON
└─ Red LED: OFF → ON

Time: 0.1s
├─ MCXC444 checks auto mode
├─ Soil is dry BUT water too low
├─ Pump activation BLOCKED
└─ Green LED stays OFF

Time: 1s
├─ MCXC444 sends status to ESP32
└─ ESP32 receives: {"soil":1,"light":0,"water":19,"pump":0,"led":0}

Time: 1.5s
├─ User opens web browser
├─ ESP32 generates HTML
├─ Detects: currentStatus.waterLevel < 20
├─ Injects RED WARNING BANNER
└─ Browser displays critical alert

Time: Continuous
├─ Buzzer keeps beeping (hardware alarm)
├─ Red LED stays ON
└─ Web page refreshes every 5s showing warning
```

## Summary

✅ **Soil Dry + Water Sufficient** → Motor releases water
🚫 **Soil Dry + Water Low** → Motor blocked, buzzer rings, web shows warning
🔴 **Water < 20%** → Always triggers alarm (buzzer + red LED + web warning)
📱 **Web Interface** → Provides visual feedback with pulsing warning banner
🔒 **Safety** → Multiple layers prevent dry-running the motor/pump

---

**The system ensures you never damage the motor by running it dry, and you're always alerted when water needs refilling!**
