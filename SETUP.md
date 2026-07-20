# Setup Guide — IoT Oxygen Concentrator Firmware

This guide walks you through setting up your computer to upload the firmware
to the ESP32. Follow every step in order. Do not skip steps.

---

## What You Need

- A computer running Windows 10/11, macOS, or Ubuntu Linux
- The ESP32 DevKit board
- A USB-A to Micro-USB cable (data cable, not charge-only)
- Internet connection for downloads

---

## Step 1 — Install Arduino IDE 2

1. Go to: https://www.arduino.cc/en/software
2. Download **Arduino IDE 2.x** for your operating system
3. Run the installer and follow the prompts
4. Launch Arduino IDE after installation

> **Windows users:** If Windows asks about installing drivers during setup, click **Install**.

---

## Step 2 — Add ESP32 Board Support

Arduino IDE does not include ESP32 support by default. You need to add it.

1. Open Arduino IDE
2. Go to **File → Preferences** (Windows/Linux) or **Arduino IDE → Preferences** (macOS)
3. Find the field labelled **"Additional boards manager URLs"**
4. Paste this URL into that field:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
5. Click **OK**
6. Go to **Tools → Board → Boards Manager**
7. In the search box, type: `esp32`
8. Find **"esp32 by Espressif Systems"** and click **Install**
9. Wait for the installation to complete (it downloads several files — this may take a few minutes)

---

## Step 3 — Install Required Libraries

Go to **Tools → Manage Libraries** (or press `Ctrl+Shift+I`).

Search for and install each library below. Click **Install** when found.
If asked to install dependencies, click **Install All**.

| Library Name | Author / Source | Purpose |
|---|---|---|
| `LiquidCrystal_I2C` | Frank de Brabander | LCD 2004 display via I2C |
| `TinyGSM` | Volodymyr Shymanskyy | SIM800L GSM communication |
| `ArduinoJson` | Benoit Blanchon | JSON parsing utility |
| `ESPAsyncWebServer` | **ESP32Async** (GitHub) | Captive portal HTTP server |
| `AsyncTCP` | **ESP32Async** (GitHub) | Required dependency of ESPAsyncWebServer |

> **Note:** FreeRTOS, WiFi, and DNSServer are already included in the ESP32 Arduino core.
> You do not need to install them separately.
>
> **Do not install the old `me-no-dev` forks** of AsyncTCP / ESPAsyncWebServer.
> Use only the maintained **ESP32Async** repositories (matching pair required):
>
> - https://github.com/ESP32Async/AsyncTCP (use **v3.4.x** or newer — must have `AsyncServer::status() const`)
> - https://github.com/ESP32Async/ESPAsyncWebServer (use **v3.11.x** or matching with the AsyncTCP above)
>
> **Arduino IDE:** Prefer **Sketch → Include Library → Add .ZIP Library** from the ESP32Async GitHub **Code → Download ZIP** (or a release zip).
> Library Manager sometimes installs mismatched or legacy packages under names like `ESP_Async_WebServer`.
> **PlatformIO:** `platformio.ini` already pins both via the ESP32Async GitHub URLs.
>
> **If you see:** `passing 'const AsyncServer' as 'this' argument discards qualifiers` on `_server.status()`
> that means **ESPAsyncWebServer is new but AsyncTCP is old/wrong**. Fix:
> 1. Delete **all** of these folders if present under `Documents/Arduino/libraries/`:
>    `ESP_Async_WebServer`, `ESPAsyncWebServer`, `AsyncTCP`, `Async_TCP`, `ESPAsyncTCP`
> 2. Reinstall **both** from ESP32Async only (same major generation — do not mix).
> 3. Restart Arduino IDE and compile again.

---

## Step 4 — Select the Correct Board and Port

1. Connect the ESP32 DevKit to your computer using the USB cable
2. In Arduino IDE, go to **Tools → Board → ESP32 Arduino**
3. Select **"ESP32 Dev Module"**
4. Go to **Tools → Port**
5. Select the port that appeared after you plugged in the ESP32
   - Windows: it will look like `COM3` or `COM4` (the number varies)
   - macOS/Linux: it will look like `/dev/ttyUSB0` or `/dev/cu.usbserial-...`

> **If no port appears:** The USB cable may be charge-only (no data wires). Try a different cable.
> On Windows, you may also need to install the CP2102 or CH340 USB driver manually —
> search for "CP2102 driver Windows" or "CH340 driver Windows" and download from the chip manufacturer.

---

## Step 5 — Configure Upload Settings

Go to **Tools** and confirm these settings:

| Setting | Value |
|---|---|
| Board | ESP32 Dev Module |
| Upload Speed | 921600 |
| CPU Frequency | 240MHz (WiFi/BT) |
| Flash Frequency | 80MHz |
| Flash Mode | QIO |
| Flash Size | 4MB (32Mb) |
| Partition Scheme | Default 4MB with spiffs |
| Core Debug Level | None |
| PSRAM | Disabled |

---

## Step 6 — Open and Upload the Firmware

1. Open the firmware folder and double-click `IoT_O2_Concentrator.ino`
   - It will open in Arduino IDE along with `config.h` and `portal_html.h` as tabs
2. Click the **Upload** button (the right-arrow icon, or press `Ctrl+U`)
3. Wait — you will see orange text scrolling in the output panel at the bottom
4. When you see **"Done uploading"**, the firmware is on the ESP32

> **Upload fails with "Failed to connect"?**
> Hold the **BOOT** button on the ESP32 DevKit while clicking Upload,
> then release it once the upload starts. Some boards require this.

---

## Step 7 — Set the Caregiver Phone Number

This can be done **any time**, before or after deployment, using whichever method is
most convenient. All three write to the same flash storage and take effect immediately.

**Option A — Web Portal (recommended, no cable needed)**
1. On your phone or laptop, connect to WiFi network **`O2-Controller`**, password **`12345678`**
2. A captive portal page should open automatically (if not, open a browser and visit `192.168.4.1`)
3. Scroll to the **Caregiver Number** card, type the number in international format
   (e.g. `+2348012345678`), and tap **Set**
4. You should see a confirmation and the "Current" field updates

**Option B — USB Serial**
1. After uploading, go to **Tools → Serial Monitor** (or press `Ctrl+Shift+M`)
2. Set the baud rate to **115200** (bottom-right dropdown in the Serial Monitor)
3. Type the command and press Enter:
   ```
   SET NUM +2348012345678
   ```
4. The system confirms the number is saved

**Option C — SMS (field update, no physical access needed)**
Send an SMS to the SIM card installed in the SIM800L EVB:
```
SET NUM +2348012345678
```
The device replies with a confirmation SMS and deletes the processed message.

The number is stored in flash memory (NVS) and survives power loss and reflashing
(as long as you don't erase flash entirely).

---

## Step 8 — Verify the System

With the ESP32 powered and running:

- The **LCD** should display O2 percentage, flow, temperature, status, GSM signal, and uptime across its 4 rows
- On boot, the system defaults to **Sensor mode**. With no real sensor connected, it runs a
  warm-up illusion — watch the O2 reading climb from ~21% to ~92% over about 15 seconds, then settle
- To test the full alert chain without hardware, connect to the **O2-Controller** WiFi portal,
  switch **Input Mode** to **WiFi**, and drag the O2 Purity slider down through each tier:
  - Below 85% → single beep, SMS sent
  - Below 70% → double beep, voice call placed
  - Below 35% → continuous tone, voice call placed
  - Below 23% (held for ~3 seconds) → continuous tone, voice call placed, **relay cuts off**
- Alerts are **silenced for the first 30 seconds after boot** — this is intentional, not a bug
- Check the Serial Monitor at 115200 baud for debug output if something is not working

---

## Troubleshooting

| Problem | Likely Cause | Fix |
|---|---|---|
| LCD is blank | Wrong I2C address | Run an I2C scanner sketch (search online: "Arduino I2C scanner"), note the address, update `LCD_I2C_ADDR` in `config.h` |
| LCD has blocks, no text | Contrast too low | Adjust the small potentiometer on the LCD backpack with a small screwdriver |
| Upload fails | Boot mode issue | Hold BOOT button on ESP32 during upload start |
| Upload fails | Wrong COM port | Unplug and replug USB, recheck Tools → Port |
| No COM port visible | Charge-only USB cable | Replace cable with a data cable |
| Compile error mentioning `AsyncServer` / `const` qualifiers | Mismatched AsyncTCP / ESPAsyncWebServer versions | See the library note in Step 3 — remove old copies, reinstall matching ESP32Async pair |
| Captive portal doesn't auto-open | Phone/OS didn't trigger detection | Manually open a browser and go to `192.168.4.1` |
| GSM not sending SMS or calling | SIM not registered on network | Check the LCD Row 4 GSM/SIM status; also check Serial Monitor for `[GSM]` log lines |
| Relay behaves opposite of expected | Wiring or pin assignment mismatch | Confirm `RELAY_PIN` and `BUZZER_PIN` in `config.h` match your actual wiring — these were swapped once during testing on this project, double-check before assuming firmware is at fault |
| Buzzer doesn't sound in WiFi demo mode | Not enough time held below threshold | Each tier needs ~3 consecutive readings (about 3 seconds) below its threshold before it triggers — this is intentional debounce, not a bug |
| Alerts don't fire right after boot | Boot delay active | Alerts are suppressed for the first 30 seconds after boot by design (`ALERT_BOOT_DELAY_MS` in `config.h`) |

---

## File Structure Reference

```
firmware/
├── IoT_O2_Concentrator.ino   # Main sketch — all 5 FreeRTOS tasks, alert state machine
├── config.h                   # All configurable constants (pins, thresholds, timing, WiFi AP)
└── portal_html.h              # Captive portal webpage (mode select, sliders, caregiver number)
```

All firmware logic lives in the single `.ino` file plus these two headers — there is no
separate `sensor.cpp` / `display.cpp` / `alert.cpp` split; each task is a function inside
the main sketch.

---

*Setup Guide — IoT Oxygen Concentrator Integration*
*Updated to match firmware with 5-tier alerts, 3 input modes, and web-based caregiver configuration.*
*For questions, refer to the full project documentation in the `docs/` folder.*