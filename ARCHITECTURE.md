# Minimal MCXC444 Firmware Architecture# System Architecture Diagram



This reset pares the MCXC444 firmware down to the two essentials requested:## Overall System Flow



- **Sensors**```

  - Soil moisture switch on `PTA1`┌─────────────────────────────────────────────────────────────────────────┐

  - LDR light sensor on `PTC2`│                         PLANT WATERING SYSTEM                           │

- **Actuators**└─────────────────────────────────────────────────────────────────────────┘

  - Water pump driver on `PTA2`

  - Indicator LED on `PTA5`┌───────────────────────────────────┐    ┌────────────────────────────────┐

│          ESP32 Module             │    │       MCXC444 Module           │

## Firmware Layout│  (Web Interface & Monitoring)     │◄──►│    (Smart Controller)          │

└───────────────────────────────────┘    └────────────────────────────────┘

```         │                                         │

source/         │ Water Level                             │ Soil Moisture

└── main.c          ← sole application module         │ Sensor (ADC)                            │ Sensor (ADC)

```         │                                         │

         │ WiFi Access                             │ Light Sensor

All previous FreeRTOS, UART, buzzer, and ESP32 integration code has been removed from the build. The remaining dependencies are only the device, board, driver, and startup support files supplied by MCUXpresso.         │ Point                                   │ (LDR + ADC)

         │                                         │

## Control Flow         │ Web Server                              │ LED Control

         │ (Port 80)                               │ (GPIO)

1. `main()` boots clocks, then configures GPIO directions and interrupt sources.         │                                         │

2. Rising and falling edges on the soil sensor (`PORTA_IRQn`) update the dry flag and drive the pump output immediately.         │                                         │ Water Pump

3. Edges on the LDR line (`PORTC_PORTD_IRQn`) update the dark flag and drive the LED output immediately.         │                                         │ Control (GPIO)

4. The main loop executes `__WFI()` forever; all work happens inside the GPIO ISRs.         │                                         │

         └─────────────────────────────────────────┘

This deliberately omits any FreeRTOS scheduler, UART messaging, or safety logic so that the behaviour is easy to observe while validating the hardware connections.                    UART Communication

                  (TX/RX @ 9600 baud)

## Debug Support```



`main.c` also defines a HardFault capture helper. If a crash occurs, the handler stores the stacked PC/stack pointer in globals and triggers a breakpoint so the faulting instruction can be identified quickly during bring-up.## Data Flow Architecture



Further features (buzzers, water-level integration, ESP32 communication, FreeRTOS tasks, etc.) can be reintroduced incrementally once the basic sensor/actuator loop is confirmed on hardware.```

┌──────────────────────────────────────────────────────────────────────┐
│                        AUTOMATIC MODE                                │
└──────────────────────────────────────────────────────────────────────┘

    [Soil Moisture Sensor] ──┐
                             │
    [LDR Light Sensor] ──────┼──► [MCXC444 ADC] ──► [Decision Logic]
                             │                            │
    [Water Level (ESP32)] ───┘                            │
                (via UART)                                ▼
                                                    ┌──────────┐
                                                    │ If Dry & │
                                                    │ Has Water│──► Pump ON
                                                    └──────────┘
                                                    ┌──────────┐
                                                    │ If Dark  │──► LED ON
                                                    └──────────┘
                                                          │
                                                          ▼
                                              [Send Status to ESP32]
                                                          │
                                                          ▼
                                                  [Display on Web]
                                                          │
                                                          ▼
                                                 [User can override]
                                                          │
                                                          ▼
                                              [Command sent to MCXC444]
```

## Module Dependencies

```
MCXC444/main.c
    ├── #include "led.h"
    ├── #include "soilMoisture.h"
    ├── #include "lightSensor.h"
    ├── #include "waterPump.h"
    └── #include "uart_comm.h"

ESP32/main.cpp
    ├── #include "waterLevel.h"
    └── #include "webServer.h"

Common/protocol.h
    └── Shared by both MCXC444 and ESP32
```

## Hardware Block Diagram

```
                    ┌─────────────┐
                    │   Power     │
                    │   Supply    │
                    │  (5V/3.3V)  │
                    └──────┬──────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
  ┌──────────┐      ┌──────────┐      ┌──────────┐
  │ MCXC444  │◄────►│  ESP32   │      │ Sensors  │
  │  (3.3V)  │ UART │  (3.3V)  │      │  (3.3V)  │
  └────┬─────┘      └────┬─────┘      └────┬─────┘
       │                 │                  │
       │                 │                  │
  ┌────┴─────┐      ┌────┴─────┐      ┌────┴─────┐
  │GPIO Pins │      │ADC Pin   │      │Analog Out│
  └────┬─────┘      │(GPIO34)  │      └──────────┘
       │            └────┬─────┘
       │                 │
       │                 ▼
       │          ┌─────────────┐
       │          │Water Level  │
       │          │   Sensor    │
       │          └─────────────┘
       │
       ├──► PTB0 ───► Soil Moisture Sensor
       ├──► PTB1 ───► LDR Light Sensor
       ├──► PTD1 ───► LED
       └──► PTD2 ───► Water Pump (via Relay)
```

## Web Interface Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    User's Device                        │
│              (Phone/Laptop/Tablet)                      │
│                                                          │
│   1. Connect to WiFi: PlantWatering_ESP32               │
│   2. Open Browser: http://192.168.4.1                   │
└────────────────────┬────────────────────────────────────┘
                     │ HTTP Request
                     ▼
┌─────────────────────────────────────────────────────────┐
│                 ESP32 Web Server                        │
│                                                          │
│  ┌────────────────────────────────────────────────┐    │
│  │  Route: /           → HTML Dashboard           │    │
│  │  Route: /cmd?c=P1   → Send "P1" to MCXC444    │    │
│  │  Route: /cmd?c=P0   → Send "P0" to MCXC444    │    │
│  └────────────────────────────────────────────────┘    │
│                                                          │
│  ┌────────────────────────────────────────────────┐    │
│  │  Receives status updates from MCXC444          │    │
│  │  Updates internal status variables             │    │
│  │  Injects values into HTML template             │    │
│  └────────────────────────────────────────────────┘    │
└────────────────────┬────────────────────────────────────┘
                     │ UART (JSON)
                     ▼
┌─────────────────────────────────────────────────────────┐
│                    MCXC444                              │
│  Receives commands, executes, sends status back         │
└─────────────────────────────────────────────────────────┘
```

## File Interaction Map

```
┌─────────────────┐
│   main.c        │◄─── Entry point (MCXC444)
└────────┬────────┘
         │ calls
         ├──► LED_Init(), LED_On(), LED_Off()      [led.c]
         ├──► SoilMoisture_Init(), _GetPercentage()[soilMoisture.c]
         ├──► LightSensor_Init(), _IsDark()        [lightSensor.c]
         ├──► WaterPump_Init(), _On(), _Off()      [waterPump.c]
         └──► UART_SendStatus(), _ReceiveCommand() [uart_comm.c]

┌─────────────────┐
│   main.cpp      │◄─── Entry point (ESP32)
└────────┬────────┘
         │ calls
         ├──► WaterLevel_Init(), _GetPercentage()  [waterLevel.c]
         └──► WebServer_Init(), _HandleClient()    [webServer.cpp]
```

## State Machine (MCXC444)

```
┌──────────────┐
│  POWER ON    │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ INITIALIZE   │
│  - LED       │
│  - Sensors   │
│  - UART      │
│  - Pump      │
└──────┬───────┘
       │
       ▼
┌──────────────────────────────┐
│      AUTO MODE               │◄──────────┐
│                              │           │
│  ┌────────────────────────┐  │           │
│  │ Read Sensors (100ms)   │  │           │
│  └───────────┬────────────┘  │           │
│              │                │           │
│  ┌───────────▼────────────┐  │           │
│  │ Check Thresholds       │  │           │
│  └───────────┬────────────┘  │           │
│              │                │           │
│  ┌───────────▼────────────┐  │           │
│  │ Control Actuators      │  │           │
│  └───────────┬────────────┘  │           │
│              │                │           │
│  ┌───────────▼────────────┐  │           │
│  │ Send Status (1000ms)   │  │           │
│  └────────────────────────┘  │           │
└──────────┬───────────────────┘           │
           │                                │
           │ Receive Command                │
           ▼                                │
┌──────────────────────────────┐           │
│     MANUAL MODE              │           │
│                              │           │
│  - Execute User Command      │           │
│  - Override Auto Logic       │           │
│  - Continue Status Updates   │───────────┘
└──────────────────────────────┘
```

## Communication Protocol Details

```
MCXC444 ──────────────────────► ESP32
        JSON Status (1 second)
        {"soil":45,"light":78,"water":80,"pump":0,"led":1}
        
ESP32 ───────────────────────► MCXC444
      Command (on button press)
      "P1\n" or "P0\n" or "L1\n" or "L0\n"

MCXC444 ──────────────────────► ESP32
        Acknowledgment
        "Pump ON (Manual)\n"
```

---

## Safety Features

1. **Max Pump Duration** (5 seconds)
   - Prevents pump from running indefinitely
   - Auto-shutoff even if sensor fails

2. **Water Level Check**
   - Only activates pump if water > 20%
   - Prevents dry-running

3. **UART Timeout**
   - MCXC444 operates independently
   - No dependency on ESP32 communication

4. **Interrupt-Driven UART**
   - Non-blocking communication
   - System remains responsive
