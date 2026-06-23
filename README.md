# IoT Oxygen Concentrator Integration

**Purity Monitoring, Alert System and Compressor Control**

A non-invasive integration module for PSA oxygen concentrators. The system monitors
oxygen purity on the outlet line, alerts a caregiver via SMS or voice call when purity
drops below safe thresholds, and cuts compressor power in critical conditions.

---

## What This System Does

| Feature | Description |
|---|---|
| Purity monitoring | OCS-3FL2.0 ultrasonic sensor on post-filtration outlet |
| Local display | LCD 2004 (I2C) shows O2%, flow, temperature, status, uptime |
| Warning alert | Below 85% - buzzer beeps + SMS to caregiver |
| Danger alert | Below 70% - persistent buzzer + voice call + relay cuts compressor |
| Caregiver config | Number stored in NVS flash, configurable via USB serial or SMS |
| Demo mode | Potentiometer on ADC pin simulates sensor while hardware is in transit |

The concentrator itself is **not modified**. All integration is external in a separate enclosure.

---

## Hardware

| Component | Role |
|---|---|
| ESP32 DevKit (standard) | Main microcontroller |
| OCS-3FL2.0 sensor | Oxygen purity, flow, temperature (UART2) |
| SIM800L GSM EVB | SMS and voice call via Nigerian GSM network (UART1) |
| LCD 2004 + I2C backpack | Local display |
| 5V relay module | Compressor power control (fail-safe NO configuration) |
| Active buzzer (5V) | Audible alert |
| LM2596 buck converter | 12V to 5V regulated rail |
| 12V AC-DC module | Mains AC to 12V DC from concentrator supply |
| 1000uF electrolytic cap | Bulk capacitance on SIM800L EVB supply |
| Potentiometer 10k | Demo sensor substitute (ADC, GPIO 34) |

Full wiring and pin assignments are documented in docs/.

---

## Project Structure

```
project-root/
├── firmware/               # Arduino sketch (.ino) and source files
├── hardware/               # KiCad schematic, PCB layout, Gerber files
├── docs/                   # Full project documentation, diagrams, images
├── README.md               # This file
├── SETUP.md                # Step-by-step setup guide for Arduino IDE
└── .gitignore
```

---

## Thresholds

| State | O2 Purity | Response |
|---|---|---|
| NORMAL | > 85% | No alert, compressor runs |
| WARNING | 70% to 85% | Buzzer (intermittent) + SMS |
| DANGER | < 70% | Buzzer (persistent) + Voice call + Relay opens |

Thresholds are defined as constants in `firmware/config.h` and can be adjusted before flashing.

---

## Firmware Architecture

The firmware uses **FreeRTOS** (built into the ESP32 Arduino core - no extra install needed).
Three tasks run concurrently, sharing sensor data through a mutex-protected struct:

- **sensorTask** - reads OCS-3FL2.0 via UART2 every 500 ms, parses packet, updates shared data
- **displayTask** - reads shared data, updates LCD 2004 every 500 ms
- **alertTask** - monitors O2 level, manages buzzer patterns, relay state, and GSM alerts

---

## Quick Start

See SETUP.md for the full step-by-step setup guide.

1. Install Arduino IDE 2.x
2. Add ESP32 board support
3. Install required libraries (listed in SETUP.md)
4. Open firmware/firmware.ino
5. Set your board to ESP32 Dev Module, port to your COM port
6. Upload

---

## Documentation

Full design documentation (Rev 1.2+) is in the docs/ folder. It covers:
- System architecture and power design
- Sensor UART protocol and packet parsing
- Relay driver circuit explanation
- Alert logic and state machine
- Caregiver number configuration (USB serial + SMS command)
- Wiring connection reference table
- Bill of Materials

---

## Caregiver Number Setup

**Before first use**, set the caregiver number via USB serial:
1. Open serial monitor at **115200 baud**
2. Enter number in international format: +2348012345678
3. Press Enter - number saves to flash and persists through power cycles

To update in the field without physical access, send an SMS to the device SIM:
```
SET:+2348012345678
```

---

*Project by Smart Ayodele.*
*Documentation Rev 1.2 - June 2026*