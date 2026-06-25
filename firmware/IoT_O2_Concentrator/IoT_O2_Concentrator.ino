#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TinyGsmClient.h>
#include "config.h"   // central config file

// LCD setup
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 20, 4);

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

static String gsmStatus       = "Idle";

SensorData sharedData;

// === Mutexes ===
SemaphoreHandle_t dataMutex;
SemaphoreHandle_t modemMutex;

// === FreeRTOS Task Handles ===
TaskHandle_t sensorTaskHandle;
TaskHandle_t displayTaskHandle;
TaskHandle_t alertTaskHandle;

// === Setup ===
void setup() {

  Serial.begin(115200);

  // Pin modes
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // compressor OFF by default
  digitalWrite(BUZZER_PIN, LOW);

  // LCD init
  lcd.init();
  lcd.backlight();

  // UART init
  simSerial.begin(GSM_BAUD, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);    // SIM EVB
  o2Serial.begin(O2_BAUD, SERIAL_8N1, O2_RX_PIN, O2_TX_PIN);        // Oxygen sensor

  // GSM modem init
  Serial.println("Initializing modem...");
  modem.restart();

  if (!modem.waitForNetwork(30000L)) {
    Serial.println("Network not found, continuing anyway...");
  }

  // Mutex init
  dataMutex  = xSemaphoreCreateMutex();
  modemMutex = xSemaphoreCreateMutex();

  // Create tasks
  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, NULL, 1, &sensorTaskHandle, 0);
  xTaskCreatePinnedToCore(displayTask, "DisplayTask", 4096, NULL, 1, &displayTaskHandle, 0);
  xTaskCreatePinnedToCore(alertTask, "AlertTask", 8192, NULL, 1, &alertTaskHandle, 1);
}

// === Sensor Task (demo mode with potentiometer) ===
void sensorTask(void *pvParameters) {

  for (;;) {
    // Demo: read potentiometer as O2 %
    int adcValue = analogRead(POT_PIN); // 0–4095
    SensorData sensor;
    sensor.o2 = O2_MIN_PERCENT + (adcValue / ADC_MAX_VALUE) * (O2_MAX_PERCENT - O2_MIN_PERCENT);
    sensor.flow = 0.0; // demo only
    sensor.temp = 25.0; // demo only
    sensor.uptime = millis() / 1000;

    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      sharedData = sensor;
      xSemaphoreGive(dataMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(SENSOR_INTERVAL_MS));
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
    if (display.o2 > O2_NORMAL_MIN) lcd.print("STATUS: NORMAL   ");
    else if (display.o2 > O2_WARNING_MIN) lcd.print("STATUS: WARNING  ");
    else lcd.print("STATUS: DANGER   ");

    int rssi = 0, simSt = 0;
    if (xSemaphoreTake(modemMutex, pdMS_TO_TICKS(500))) {
        rssi  = modem.getSignalQuality();
        simSt = modem.getSimStatus();
        xSemaphoreGive(modemMutex);
    }

    lcd.setCursor(0, 3);
    lcd.print("GSM: ");
    lcd.print(rssi);
    lcd.print(" SIM: ");
    lcd.print(simSt == 1 ? "OK  " : "FAIL");
    lcd.print(" ");
    lcd.print(gsmStatus);

    vTaskDelay(pdMS_TO_TICKS(DISPLAY_INTERVAL_MS));
  }
}

// === Alert Task ===
void alertTask(void *pvParameters) {
  static uint8_t dangerCount    = 0;
  static uint8_t warningCount   = 0;
  static unsigned long lastBuzz = 0;

  static bool smsSent           = false;
  static bool callMade          = false;

  for (;;) {
    SensorData alert;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      alert = sharedData;
      xSemaphoreGive(dataMutex);
    }

    if (alert.o2 < O2_WARNING_MIN) {
      dangerCount++;
      warningCount  = 0;
    }
    else if (alert.o2 < O2_NORMAL_MIN) {
      warningCount++;
      dangerCount   = 0;
    }
    else {
      dangerCount   = 0;
      warningCount  = 0;
      smsSent       = false;
      callMade      = false;
      gsmStatus     = "Idle";
    }

    if (dangerCount >= 3) {
      digitalWrite(RELAY_PIN, HIGH);  // compressor off
      digitalWrite(BUZZER_PIN, HIGH); // continous buzzing
      if (!callMade) {
        if (modem.isNetworkConnected()) {
          if (xSemaphoreTake(modemMutex, pdMS_TO_TICKS(2000))) {
            gsmStatus  = "Calling...";
            if (modem.callNumber(CAREGIVER_NUMBER)) {
              callMade = true;
              gsmStatus = "Call Sent";
              Serial.println("Call Sent");
            } else {
              gsmStatus = "Call Retry";
              Serial.println("Call failed, will retry...");
            }
          }
        } else {
          gsmStatus = "Call Waiting Net...";
          Serial.println("Network not connected, retrying call...");
        }
      }
    }
    else if (warningCount >= 3) {
      digitalWrite(RELAY_PIN, LOW); // compressor still on
      // WARNING: short intermittent beeps
      if (millis() - lastBuzz > BUZZER_WARNING_GAP) {
        digitalWrite(BUZZER_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(BUZZER_WARNING_MS));
        digitalWrite(BUZZER_PIN, LOW);
        lastBuzz = millis();
      }
      if (!smsSent) {
        if (modem.isNetworkConnected()) {
          if (xSemaphoreTake(modemMutex, pdMS_TO_TICKS(2000))) {
            gsmStatus = "SMS Pending...";
            String msg = "ALERT: O2 purity low. Reading: ";
            msg += String(alert.o2, 1);
            msg += "%. Please check the concentrator.";
            if (modem.sendSMS(CAREGIVER_NUMBER, msg.c_str())) {
              smsSent = true;
              gsmStatus = "SMS Sent";
              Serial.println("SMS sent successfully.");
            } else {
              gsmStatus = "SMS Retry...";
              Serial.println("SMS failed, will retry...");
            }
          }
        } else {
          gsmStatus = "SMS Waiting Net...";
          Serial.println("Network not connected, retrying sms...");
        }
      }
    }
    else {
      digitalWrite(RELAY_PIN, LOW);   // compressor on in normal operation
      digitalWrite(BUZZER_PIN, LOW);  // buzzer off in normal operation
    }

    vTaskDelay(pdMS_TO_TICKS(ALERT_INTERVAL_MS));
  }
}

void loop() {
  vTaskDelay(portMAX_DELAY); // everything runs in tasks
}
