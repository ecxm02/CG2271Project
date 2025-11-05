# 🎯 ONE-PAGE QUICK REFERENCE

## System at a Glance

```
╔══════════════════════════════════════════════════════════════════╗
║                  PLANT WATERING SYSTEM LOGIC                     ║
╠══════════════════════════════════════════════════════════════════╣
║                                                                  ║
║  WATER ≥ 20% + SOIL DRY  →  💧 PUMP ON  →  Plant watered ✅     ║
║                                                                  ║
║  WATER < 20% + SOIL DRY  →  🚫 PUMP OFF  →  ALARM! 🚨           ║
║                              ├─ 📢 Buzzer beeping               ║
║                              ├─ 🔴 Red LED on                   ║
║                              └─ 🌐 Web warning banner           ║
║                                                                  ║
╚══════════════════════════════════════════════════════════════════╝
```

## Web Interface

### Normal (Water OK)
```
┌────────────────────────────┐
│ 🌱 Plant Watering System   │
│ Soil: 0% [DRY]            │
│ Light: 100% [BRIGHT]      │
│ Water: 75% ✅              │
└────────────────────────────┘
```

### Alert (Water Low)
```
┌────────────────────────────┐
│ 🌱 Plant Watering System   │
│ ⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️   │
│ WATER LEVEL TOO LOW!      │
│ REFILL IMMEDIATELY        │
│ 🔴 Buzzer | 🚫 Disabled   │
│ ⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️   │
│ Soil: 0% [DRY]            │
│ Light: 100% [BRIGHT]      │
│ Water: 15% 🚨              │
└────────────────────────────┘
```

## LED Indicators (MCXC444)

| LED | ON = | OFF = |
|-----|------|-------|
| 🔴 Red | Water < 20% (ALARM) | Water OK |
| 🟢 Green | Pump running | Pump idle |
| 🔵 Blue | Dark/Night | Bright/Day |

## Hardware

```
MCXC444 Pins:
├─ PTB0  → Soil sensor (1=dry, 0=wet)
├─ PTE30 → Light sensor (1=dark, 0=bright)
├─ PTD2  → Motor/Pump
├─ PTD3  → Buzzer 🆕
└─ UART  → ESP32 (9600 baud)
S
ESP32 Pins:
├─ GPIO34 → Water level (ADC 0-100%)
├─ UART   → MCXC444
└─ WiFi   → Web interface
```

## Access

**WiFi:** PlantWatering_ESP32  
**Password:** 12345678  
**URL:** http://192.168.4.1

## Emergency Actions

**Buzzer won't stop?**  
→ Water level < 20%, refill reservoir

**Pump won't turn on?**  
→ Check water level, must be > 20%

**Web shows warning?**  
→ Refill water immediately

## Auto-Refresh
Web page updates every 5 seconds automatically.

---
**Ready to use! 🚀**
