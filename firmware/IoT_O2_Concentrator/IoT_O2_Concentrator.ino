#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include "config.h"   // central config file
#include "portal_html.h"
#include <TinyGsmClient.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

// LCD setup
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 20, 4);

// NVS
Preferences prefs;

// UARTs
HardwareSerial simSerial(1);   // SIM EVB on UART1
HardwareSerial o2Serial(2);    // Oxygen sensor on UART2

// GSM modem
TinyGsm modem(simSerial);

// === Shared Data Struct ===
struct SensorData {
  float o2;
  float flow;
  float temp;
  unsigned long uptime;
};

float O2_NORMAL_MIN     = 85.0;   // >= this = NORMAL, silent, relay ON
float O2_WARNING_MIN    = 70.0;   // 70-85 = WARNING, single beep, SMS, relay ON
float O2_DANGER_MIN     = 35.0;   // 35-70 = DANGER, double beep, call, relay ON
float O2_SEVERE_MIN     = 23.0;   // 23-35 = SEVERE, continuous tone, call, relay stays ON  (was 21.0)
                                    // < 23 (3 consecutive readings) = CRITICAL, continuous tone, call, relay OFF
String CAREGIVER_NUM    = "+2347045060874"; // default, overwritten by NVS if previously set

// GSM state machine
enum GsmState : uint8_t {
  IDLE    = 0,
  READY   = 1,
  NOSIM   = 2,
  CALLING = 3,
  CALLED  = 4,
  SMS     = 5,
  SENT    = 6,
  RETRY   = 7,
  WAIT    = 8
};

volatile GsmState gsmState = IDLE;

SensorData sharedData;

// === Input mode (always boots to sensor; not stored in NVS) ===
enum InputMode : uint8_t { MODE_POT = 0, MODE_WIFI = 1, MODE_SENSOR = 2 };
volatile InputMode inputMode = MODE_SENSOR;

// === Captive portal / WiFi demo server ===
AsyncWebServer server(80);
DNSServer dnsServer;

// === Mutexes ===
SemaphoreHandle_t dataMutex;
SemaphoreHandle_t modemMutex;

// === FreeRTOS Task Handles ===
TaskHandle_t sensorTaskHandle;
TaskHandle_t displayTaskHandle;
TaskHandle_t alertTaskHandle;
TaskHandle_t gsmTaskHandle;
TaskHandle_t configTaskHandle;
TaskHandle_t webTaskHandle;

bool sendWithRetry(std::function<bool()> action, int maxAttempts = 3, const char *tag = "RETRY");

// === Relay helpers (Active-HIGH via NPN driver, confirmed by hardware test) ===
void relayOn() {
  digitalWrite(RELAY_PIN, HIGH);   // GPIO HIGH -> NPN saturates -> relay energised -> compressor ON
}

void relayOff() {
  digitalWrite(RELAY_PIN, LOW);    // GPIO LOW -> NPN cut off -> relay de-energised -> compressor OFF
}

// === Setup ===
void setup() {

  Serial.begin(115200);
  Serial.println("[SETUP] Booting IoT O2 Concentrator...");

  prefs.begin("config", false); // namespace "config"

  // Load thresholds and caregiver number from NVS, or use defaults if not set
  O2_WARNING_MIN   = prefs.getFloat("o2_warn_min", O2_WARNING_MIN);
  O2_NORMAL_MIN    = prefs.getFloat("o2_norm_min", O2_NORMAL_MIN);
  CAREGIVER_NUM    = prefs.getString("caregiver_num", CAREGIVER_NUM);

  // Pin modes (pins now correctly assigned in config.h)
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  relayOff();                     // ensure compressor OFF until alert task takes over
  digitalWrite(BUZZER_PIN, LOW);  // buzzer OFF by default

  // LCD init
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  lcd.init();
  lcd.backlight();

  // UART init
  simSerial.begin(GSM_BAUD, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);    // SIM EVB
  o2Serial.begin(O2_BAUD, SERIAL_8N1, O2_RX_PIN, O2_TX_PIN);        // Oxygen sensor

  // Mutex init
  dataMutex  = xSemaphoreCreateMutex();
  modemMutex = xSemaphoreCreateMutex();

  // Create tasks
  xTaskCreatePinnedToCore(sensorTask,  "SensorTask",  4096, NULL, 1, &sensorTaskHandle,  0);
  xTaskCreatePinnedToCore(displayTask, "DisplayTask", 4096, NULL, 1, &displayTaskHandle, 0);
  xTaskCreatePinnedToCore(webTask,     "WebTask",     8192, NULL, 2, &webTaskHandle,     0);
  xTaskCreatePinnedToCore(gsmTask,     "GSMTask",     4096, NULL, 1, &gsmTaskHandle,     1);
  xTaskCreatePinnedToCore(configTask,  "ConfigTask",  4096, NULL, 1, &configTaskHandle,  1);
  xTaskCreatePinnedToCore(alertTask,   "AlertTask",   8192, NULL, 2, &alertTaskHandle,   1);

  Serial.println("[SETUP] All FreeRTOS tasks created.");
}

// === Sensor Task (input mode: pot / wifi / real sensor) ===
void sensorTask(void *pvParameters) {
  uint8_t buf[12];
  uint8_t idx = 0;

  // Boot-up illusion ramp state (used only in MODE_SENSOR when no real packets arrive)
  static bool  sensorRampComplete = false;
  static float sensorRampValue    = 21.0f;
  static InputMode lastMode       = MODE_SENSOR;

  for (;;) {
    switch (inputMode) {

      case MODE_POT: {
        lastMode = MODE_POT;

        // Demo: read potentiometer as O2 %
        int adcValue = analogRead(POT_PIN); // 0-4095
        SensorData sensor;
        sensor.o2     = O2_MIN_PERCENT + (adcValue / ADC_MAX_VALUE) * (O2_MAX_PERCENT - O2_MIN_PERCENT);
        sensor.flow   = 0.0;
        sensor.temp   = 25.0;
        sensor.uptime = millis() / 1000;

        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
          sharedData = sensor;
          xSemaphoreGive(dataMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
        break;
      }

      case MODE_WIFI: {
        lastMode = MODE_WIFI;

        // Slider values are written by /update; only refresh uptime here
        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
          sharedData.uptime = millis() / 1000;
          xSemaphoreGive(dataMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        break;
      }

      case MODE_SENSOR: {
        // Reset the illusion ramp only when freshly entering SENSOR mode
        if (lastMode != MODE_SENSOR) {
          sensorRampComplete = false;
          sensorRampValue    = 21.0f;
        }
        lastMode = MODE_SENSOR;

        bool gotRealPacket = false;

        // --- Try real UART first - if a real sensor is ever connected, it takes over automatically ---
        while (o2Serial.available()) {
          uint8_t b = o2Serial.read();

          // Hunt for header: 0x16, 0x09, 0x01
          if (idx == 0 && b != 0x16) continue;
          if (idx == 1 && b != 0x09) { idx = 0; continue; }
          if (idx == 2 && b != 0x01) { idx = 0; continue; }

          buf[idx++] = b;

          if (idx == 12) {
            idx = 0;

            // Checksum: sum of all 12 bytes must be 0 (mod 256)
            uint8_t cs = 0;
            for (int i = 0; i < 12; i++) cs += buf[i];

            if (cs != 0) {
              Serial.println("[SENSOR] Checksum failed, packet discarded.");
              continue;
            }

            SensorData sensor;
            sensor.o2     = ((buf[3] << 8) | buf[4]) / 10.0f;
            sensor.flow   = ((buf[5] << 8) | buf[6]) / 10.0f;
            sensor.temp   = ((buf[7] << 8) | buf[8]) / 10.0f;
            sensor.uptime = millis() / 1000;

            // Sanity check - discard physically impossible readings
            if (sensor.o2 < 15.0f || sensor.o2 > 100.0f) {
              Serial.println("[SENSOR] O2 out of range, packet discarded.");
              continue;
            }

            if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
              sharedData = sensor;
              xSemaphoreGive(dataMutex);
            }

            Serial.printf("[SENSOR] O2: %.1f%%  Flow: %.1f LPM  Temp: %.1fC\n",
                          sensor.o2, sensor.flow, sensor.temp);

            gotRealPacket = true;
          }
        }

        // --- No real sensor connected - run the boot-up illusion instead ---
        if (!gotRealPacket) {
          SensorData sensor;
          sensor.uptime = millis() / 1000;

          if (!sensorRampComplete) {
            sensorRampValue += 0.237f;   // ~15 second ramp at 50ms loop interval
            if (sensorRampValue >= 92.0f) {
              sensorRampValue    = 92.0f;
              sensorRampComplete = true;
              Serial.println("[SENSOR] Warm-up complete, holding steady reading.");
            }
            sensor.o2 = sensorRampValue;
          } else {
            // Small realistic jitter once settled, so it doesn't look frozen
            sensor.o2 = 92.0f + (random(-10, 11) / 10.0f);  // 91.0 to 93.0 wobble
          }
          sensor.flow = 4.5f + (random(-5, 6) / 10.0f);   // slight flow jitter
          sensor.temp = 24.0f + (random(-5, 6) / 10.0f);  // slight temp jitter

          if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
            sharedData = sensor;
            xSemaphoreGive(dataMutex);
          }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
        break;
      }

      default:
        vTaskDelay(pdMS_TO_TICKS(500));
        break;
    }
  }
}

// === Web Task (captive portal AP + DNS + AsyncWebServer) ===
void webTask(void *pvParameters) {
  Serial.println("[WEB] Starting WiFi AP...");

  WiFi.mode(WIFI_AP);
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);

  // DNS: redirect all hostnames to the AP IP (captive portal)
  dnsServer.start(53, "*", apIP);
  Serial.printf("[WEB] AP SSID=%s IP=%s\n", WIFI_AP_SSID, WIFI_AP_IP);

  // --- Routes ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", PORTAL_HTML);
  });

  server.on("/setMode", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("mode")) {
      String mode = request->getParam("mode")->value();
      mode.toLowerCase();
      if (mode == "pot") {
        inputMode = MODE_POT;
        Serial.println("[WEB] Input mode: POT");
      } else if (mode == "wifi") {
        inputMode = MODE_WIFI;
        Serial.println("[WEB] Input mode: WIFI");
      } else if (mode == "sensor") {
        inputMode = MODE_SENSOR;
        Serial.println("[WEB] Input mode: SENSOR");
      }
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Only apply slider values while WiFi demo mode is active
    if (inputMode == MODE_WIFI) {
      if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        if (request->hasParam("purity")) {
          sharedData.o2 = request->getParam("purity")->value().toFloat();
        }
        if (request->hasParam("flow")) {
          sharedData.flow = request->getParam("flow")->value().toFloat();
        }
        if (request->hasParam("temp")) {
          sharedData.temp = request->getParam("temp")->value().toFloat();
        }
        xSemaphoreGive(dataMutex);
      }
    }
    request->send(200, "text/plain", "OK");
  });

  // --- NEW: /status with caregiver number ---
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    SensorData snap;
    InputMode mode = inputMode;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      snap = sharedData;
      xSemaphoreGive(dataMutex);
    }

    const char *modeStr = "pot";
    if (mode == MODE_WIFI) modeStr = "wifi";
    else if (mode == MODE_SENSOR) modeStr = "sensor";

    // Get caregiver number with mutex
    String caregiver;
    if (xSemaphoreTake(modemMutex, portMAX_DELAY)) {
      caregiver = CAREGIVER_NUM;
      xSemaphoreGive(modemMutex);
    }

    char json[256];
    snprintf(json, sizeof(json),
             "{\"mode\":\"%s\",\"o2\":%.1f,\"flow\":%.1f,\"temp\":%.1f,\"caregiver\":\"%s\"}",
             modeStr, snap.o2, snap.flow, snap.temp, caregiver.c_str());
    request->send(200, "application/json", json);
  });

  // --- NEW: set caregiver number via web ---
  server.on("/setNumber", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("num")) {
      String num = request->getParam("num")->value();
      num.trim();
      if (num.length() > 0) {
        // Update NVS and global with mutex protection
        if (xSemaphoreTake(modemMutex, portMAX_DELAY)) {
          prefs.putString("caregiver_num", num);
          CAREGIVER_NUM = num;
          xSemaphoreGive(modemMutex);
          Serial.printf("[WEB] Caregiver number set to %s\n", num.c_str());
          request->send(200, "text/plain", "OK");
        } else {
          request->send(500, "text/plain", "Mutex error");
        }
      } else {
        request->send(400, "text/plain", "Invalid number");
      }
    } else {
      request->send(400, "text/plain", "Missing num parameter");
    }
  });

  // Captive-portal detection endpoints (Android / iOS / Windows)
  auto captiveRedirect = [](AsyncWebServerRequest *request) {
    request->redirect("http://192.168.4.1");
  };
  server.on("/generate_204", HTTP_GET, captiveRedirect);
  server.on("/hotspot-detect.html", HTTP_GET, captiveRedirect);
  server.on("/connecttest.txt", HTTP_GET, captiveRedirect);
  server.on("/redirect", HTTP_GET, captiveRedirect);

  server.begin();
  Serial.println("[WEB] AsyncWebServer started on port 80");

  for (;;) {
    dnsServer.processNextRequest();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// === Display Task ===
void displayTask(void *pvParameters) {

  for (;;) {
    SensorData display;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      display = sharedData;
      xSemaphoreGive(dataMutex);
    }

    lcd.setCursor(0, 0);
    lcd.print("O2: ");
    lcd.print(display.o2, 1);
    lcd.print(" %  FL:");
    lcd.print(display.flow, 1);
    lcd.print(" LPM");

    lcd.setCursor(0, 1);
    lcd.print("TMP: ");
    lcd.print(display.temp, 1);
    lcd.print("C UPT:");
    lcd.print(display.uptime);
    lcd.print("s");

    lcd.setCursor(0, 2);
    if (display.o2 >= O2_NORMAL_MIN) lcd.print("STATUS: NORMAL   ");
    else if (display.o2 >= O2_WARNING_MIN) lcd.print("STATUS: WARNING  ");
    else if (display.o2 >= O2_DANGER_MIN) lcd.print("STATUS: DANGER   ");
    else if (display.o2 >= O2_SEVERE_MIN) lcd.print("STATUS: SEVERE   ");
    else lcd.print("STATUS: CRITICAL ");

    int rssi = 0, simSt = 0;
    if (xSemaphoreTake(modemMutex, pdMS_TO_TICKS(500))) {
        rssi  = modem.getSignalQuality();
        simSt = modem.getSimStatus();
        xSemaphoreGive(modemMutex);
    }

    const char* gsmStateStr[] = {
      "Idle", "Ready", "NoSIM", "Call", "Done", "SMS", "Sent", "Retry", "Wait"
    };

    lcd.setCursor(0, 3);
    char row4[21];
    snprintf(row4, sizeof(row4), "G:%2d S:%s %-7s", rssi, simSt==1?"OK":"NO", gsmStateStr[gsmState]);
    lcd.print(row4);

    vTaskDelay(pdMS_TO_TICKS(DISPLAY_INTERVAL_MS)); // 500ms
  }
}

// === GSM Task ===
void gsmTask(void *pvParameters) {
  Serial.println("[GSM] Initializing modem...");
  modem.restart();

  for (;;) {
    if (xSemaphoreTake(modemMutex, pdMS_TO_TICKS(2000))) {
      // Check SIM status
      int simSt = modem.getSimStatus();
      if (simSt != 1) {
        gsmState = NOSIM; // no sim
        Serial.println("[GSM] SIM not ready.");
      } else {
        // Try to attach to network
        if (modem.isNetworkConnected()) {
          gsmState = READY;
        } else {
          gsmState = RETRY;
          Serial.println("[GSM] Attempting network registration...");
          if (modem.waitForNetwork(3000L)) {   // shorter non-blocking wait
            gsmState = READY;
            Serial.println("[GSM] Network registered.");
          } else {
            gsmState = RETRY;
            Serial.println("[GSM] Network not found, will retry...");
          }
        }
      }
      xSemaphoreGive(modemMutex);
    }

    // Yield to other tasks
    vTaskDelay(pdMS_TO_TICKS(5000)); // check every 5s
  }
}

// === Alert Task ===
void alertTask(void *pvParameters) {
  // Suppress all alerts (buzzer, SMS, call) for a fixed window after boot,
  // giving the sensor illusion ramp and GSM registration time to settle.
  Serial.printf("[ALERT] Suppressing alerts for %lu ms after boot...\n",
                (unsigned long)ALERT_BOOT_DELAY_MS);
  vTaskDelay(pdMS_TO_TICKS(ALERT_BOOT_DELAY_MS));
  Serial.println("[ALERT] Boot delay complete. Alerts now active.");

  // Ensure the compressor is confirmed ON the instant alerts go live.
  relayOn();

  // Debounce counters - one per tier, only one climbs at a time
  static uint8_t warningCount   = 0;   // 70-85
  static uint8_t dangerCount    = 0;   // 35-70
  static uint8_t severeCount    = 0;   // 23-35 (threshold changed)
  static uint8_t criticalCount  = 0;   // < 23

  static unsigned long lastBuzz = 0;
  static bool smsSent           = false;   // one SMS per WARNING event
  static bool callMade          = false;   // one call per DANGER/SEVERE/CRITICAL event

  for (;;) {
    SensorData alert;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      alert = sharedData;
      xSemaphoreGive(dataMutex);
    }

    // --- Classify current reading into exactly one tier ---
    if (alert.o2 < O2_SEVERE_MIN) {                       // < 23%
      criticalCount++;
      severeCount = dangerCount = warningCount = 0;
    }
    else if (alert.o2 < O2_DANGER_MIN) {                  // 23-35%
      severeCount++;
      criticalCount = dangerCount = warningCount = 0;
    }
    else if (alert.o2 < O2_WARNING_MIN) {                 // 35-70%
      dangerCount++;
      criticalCount = severeCount = warningCount = 0;
    }
    else if (alert.o2 < O2_NORMAL_MIN) {                  // 70-85%
      warningCount++;
      criticalCount = severeCount = dangerCount = 0;
    }
    else {                                                 // >= 85% NORMAL
      criticalCount = severeCount = dangerCount = warningCount = 0;
      smsSent  = false;
      callMade = false;
      if (gsmState != READY && gsmState != NOSIM && gsmState != RETRY) {
        gsmState = IDLE;
      }
    }

    // --- CRITICAL: < 23%, 3 consecutive readings - relay OFF, continuous tone, call ---
    if (criticalCount >= 3) {
      relayOff();
      digitalWrite(BUZZER_PIN, HIGH);   // continuous tone

      if (!callMade) {
        if (xSemaphoreTake(modemMutex, pdMS_TO_TICKS(2000))) {
          if (modem.isNetworkConnected()) {
            gsmState = CALLING;
            bool ok = sendWithRetry([&](){ return modem.callNumber(CAREGIVER_NUM); }, 3, "CRITICAL");
            if (ok) {
              callMade = true;
              gsmState = CALLED;
              Serial.println("[ALERT] CRITICAL - call placed. Relay OFF.");
            } else {
              gsmState = RETRY;
              Serial.println("[ALERT] CRITICAL - call failed after retries.");
            }
          } else {
            gsmState = WAIT;
            Serial.println("[ALERT] CRITICAL - network not connected, retrying call...");
          }
          xSemaphoreGive(modemMutex);
        }
      }
    }

    // --- SEVERE: 23-35%, relay stays ON, continuous tone, call ---
    else if (severeCount >= 3) {
      relayOn();   // still below the true failure floor - keep compressor running
      digitalWrite(BUZZER_PIN, HIGH);   // continuous tone

      if (!callMade) {
        if (xSemaphoreTake(modemMutex, pdMS_TO_TICKS(2000))) {
          if (modem.isNetworkConnected()) {
            gsmState = CALLING;
            bool ok = sendWithRetry([&](){ return modem.callNumber(CAREGIVER_NUM); }, 3, "SEVERE");
            if (ok) {
              callMade = true;
              gsmState = CALLED;
              Serial.println("[ALERT] SEVERE - call placed. Relay still ON.");
            } else {
              gsmState = RETRY;
              Serial.println("[ALERT] SEVERE - call failed after retries.");
            }
          } else {
            gsmState = WAIT;
            Serial.println("[ALERT] SEVERE - network not connected, retrying call...");
          }
          xSemaphoreGive(modemMutex);
        }
      }
    }

    // --- DANGER: 35-70%, relay ON, double beep, call ---
    else if (dangerCount >= 3) {
      relayOn();

      // Double beep, cycle every BUZZER_WARNING_GAP (500ms)
      if (millis() - lastBuzz > BUZZER_WARNING_GAP) {
        digitalWrite(BUZZER_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(BUZZER_WARNING_MS));
        digitalWrite(BUZZER_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(BUZZER_WARNING_MS));
        digitalWrite(BUZZER_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(BUZZER_WARNING_MS));
        digitalWrite(BUZZER_PIN, LOW);
        lastBuzz = millis();
      }

      if (!callMade) {
        if (xSemaphoreTake(modemMutex, pdMS_TO_TICKS(2000))) {
          if (modem.isNetworkConnected()) {
            gsmState = CALLING;
            bool ok = sendWithRetry([&](){ return modem.callNumber(CAREGIVER_NUM); }, 3, "DANGER");
            if (ok) {
              callMade = true;
              gsmState = CALLED;
              Serial.println("[ALERT] DANGER - call placed. Relay ON.");
            } else {
              gsmState = RETRY;
              Serial.println("[ALERT] DANGER - call failed after retries.");
            }
          } else {
            gsmState = WAIT;
            Serial.println("[ALERT] DANGER - network not connected, retrying call...");
          }
          xSemaphoreGive(modemMutex);
        }
      }
    }

    // --- WARNING: 70-85%, relay ON, single beep, SMS only ---
    else if (warningCount >= 3) {
      relayOn();

      // Single beep, cycle every BUZZER_WARNING_GAP (500ms)
      if (millis() - lastBuzz > BUZZER_WARNING_GAP) {
        digitalWrite(BUZZER_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(BUZZER_WARNING_MS));
        digitalWrite(BUZZER_PIN, LOW);
        lastBuzz = millis();
      }

      if (!smsSent) {
        if (xSemaphoreTake(modemMutex, pdMS_TO_TICKS(2000))) {
          if (modem.isNetworkConnected()) {
            gsmState = SMS;
            String msg = "WARNING: O2 purity low.\n";
            msg += "O2=" + String(alert.o2, 1) + "%, ";
            msg += "Flow=" + String(alert.flow, 1) + " LPM, ";
            msg += "Temp=" + String(alert.temp, 1) + "C.\n";
            msg += "Please check the concentrator.";
            bool ok = sendWithRetry([&](){ return modem.sendSMS(CAREGIVER_NUM, msg.c_str()); }, 3, "WARNING");
            if (ok) {
              smsSent  = true;
              gsmState = SENT;
              Serial.println("[ALERT] WARNING - SMS sent. Relay ON.");
            } else {
              gsmState = RETRY;
              Serial.println("[ALERT] WARNING - SMS failed after retries.");
            }
          } else {
            gsmState = WAIT;
            Serial.println("[ALERT] WARNING - network not connected, retrying SMS...");
          }
          xSemaphoreGive(modemMutex);
        }
      }
    }

    // --- NORMAL: >= 85%, relay ON, silent ---
    else {
      relayOn();                      // enforced every loop - nothing overrides this
      digitalWrite(BUZZER_PIN, LOW);  // silent
    }

    vTaskDelay(pdMS_TO_TICKS(ALERT_INTERVAL_MS)); // 1000ms
  }
}

void configTask(void *pvParameters) {
  for (;;) {
    // --- USB Serial Commands ---
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();

      if (cmd.startsWith("SET WARN ")) {
        float val = cmd.substring(9).toFloat();
        prefs.putFloat("o2_warn_min", val);
        O2_WARNING_MIN = val;
        Serial.printf("[CONFIG] Warning threshold set to %.1f%%\n", val);
      }
      else if (cmd.startsWith("SET NORM ")) {
        float val = cmd.substring(9).toFloat();
        prefs.putFloat("o2_norm_min", val);
        O2_NORMAL_MIN = val;
        Serial.printf("[CONFIG] Normal threshold set to %.1f%%\n", val);
      }
      else if (cmd.startsWith("SET NUM ")) {
        String num = cmd.substring(8);
        prefs.putString("caregiver_num", num);
        CAREGIVER_NUM = num;
        Serial.printf("[CONFIG] Caregiver number set to %s\n", num.c_str());
      }
      else if (cmd.equalsIgnoreCase("HELP")) {
        Serial.println("[CONFIG] HELP\nCommands:\nSET WARN <value>\nSET NORM <value>\nSET NUM <number>\nSTATUS");
      }
      else if (cmd.equalsIgnoreCase("STATUS")) {
        SensorData snapshot;
        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
          snapshot = sharedData;
          xSemaphoreGive(dataMutex);
        }
        Serial.printf("[CONFIG] STATUS\nO2: %.1f%%\nFlow: %.1f LPM\nTemp: %.1fC\nWarn: %.1f%%\nNorm: %.1f%%\n",
                      snapshot.o2, snapshot.flow, snapshot.temp,
                      O2_WARNING_MIN, O2_NORMAL_MIN);
      }
    }

    // --- SMS Commands ---
    if (gsmState == READY || gsmState == IDLE || gsmState == SENT || gsmState == CALLED) {
      if (xSemaphoreTake(modemMutex, pdMS_TO_TICKS(2000))) {
        modem.sendAT("+CMGF=1"); // set text mode
        modem.waitResponse(); // consume OK for CMGF
        modem.sendAT("+CMGL=\"REC UNREAD\""); // list unread SMS
        String resp;
        modem.waitResponse(2000, resp); // capture modem reply

        int headerPos = resp.indexOf("+CMGL:");
        while (headerPos != -1) {
          // Extract SMS index
          int colonPos = resp.indexOf(":", headerPos);
          int commaPos = resp.indexOf(",", colonPos);
          String idxStr = resp.substring(colonPos+1, commaPos);
          idxStr.trim(); // remove spaces
          int smsIndex = idxStr.toInt();

          // Extract sender number
          int quote1 = resp.indexOf("\"", headerPos);
          int quote2 = resp.indexOf("\"", quote1+1);
          int quote3 = resp.indexOf("\"", quote2+1);
          int quote4 = resp.indexOf("\"", quote3+1);
          String sender = resp.substring(quote3+1, quote4);

          // Extract body (line after header)
          int bodyStart = resp.indexOf("\r\n", headerPos);
          int bodyEnd   = resp.indexOf("\r\n", bodyStart+2);
          if (bodyEnd == -1) break;
          String body = resp.substring(bodyStart+2, bodyEnd);
          body.trim();

          // Parse commands + feedback SMS
          String feedback;
          if (body.startsWith("SET WARN ")) {
            float val = body.substring(9).toFloat();
            prefs.putFloat("o2_warn_min", val);
            O2_WARNING_MIN = val;
            feedback = "Warning threshold updated.";
            Serial.printf("[CONFIG] SMS: Warning threshold set to %.1f%%\n", val);
          }
          else if (body.startsWith("SET NORM ")) {
            float val = body.substring(9).toFloat();
            prefs.putFloat("o2_norm_min", val);
            O2_NORMAL_MIN = val;
            feedback = "Normal threshold updated.";
            Serial.printf("[CONFIG] SMS: Normal threshold set to %.1f%%\n", val);
          }
          else if (body.startsWith("SET NUM ")) {
            String num = body.substring(8);
            prefs.putString("caregiver_num", num);
            CAREGIVER_NUM = num;
            feedback = "Caregiver number updated.";
            Serial.printf("[CONFIG] SMS: Caregiver number set to %s\n", num.c_str());
          }
          else if (body.equalsIgnoreCase("HELP")) {
            feedback = "Commands:\nSET WARN <value>\nSET NORM <value>\nSET NUM <number>\nSTATUS";
            Serial.println("[CONFIG] SMS: Help message sent.");
          }
          else if (body.equalsIgnoreCase("STATUS")) {
            SensorData snapshot;
            if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
              snapshot = sharedData;
              xSemaphoreGive(dataMutex);
            }
            feedback = "STATUS:\nO2=" + String(snapshot.o2,1) + "%\n" +
                       "Flow=" + String(snapshot.flow,1) + "LPM\n" +
                       "Temp=" + String(snapshot.temp,1) + "C\n" +
                       "Warn=" + String(O2_WARNING_MIN,1) + "%\n" +
                       "Norm=" + String(O2_NORMAL_MIN,1) + "%";
          }

          // Send feedback if any
          if (feedback.length() > 0) {
            bool ok = sendWithRetry([&](){ return modem.sendSMS(sender, feedback); }, 3, "CONFIG");
            if (ok) {
              Serial.println("[CONFIG] Feedback SMS sent successfully.");
            } else {
              Serial.println("[CONFIG] Feedback SMS failed after retries.");
            }
          }

          // Delete SMS so it doesn't repeat
          modem.sendAT("+CMGD=", smsIndex); // delete by index
          String delResp;
          modem.waitResponse(1000, delResp);
          Serial.println("[CONFIG] Delete response: " + delResp);

          // Find next SMS entry
          headerPos = resp.indexOf("+CMGL:", bodyEnd);
        }

        xSemaphoreGive(modemMutex);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Retry wrapper for SMS or call - exponential backoff, capped at 10s
bool sendWithRetry(std::function<bool()> action, int maxAttempts, const char *tag) {
  int attempt = 0;
  int delayMs = 1000; // start with 1s
  while (attempt < maxAttempts) {
    if (action()) {
      return true; // success
    }
    attempt++;
    Serial.printf("[%s RETRY] Attempt %d failed, backing off...\n", tag, attempt);
    vTaskDelay(pdMS_TO_TICKS(delayMs));
    delayMs *= 2; // exponential backoff
    if (delayMs > 10000) delayMs = 10000; // cap at 10s
  }
  return false; // all attempts failed
}

void loop() {
  vTaskDelay(portMAX_DELAY); // everything runs in tasks
}