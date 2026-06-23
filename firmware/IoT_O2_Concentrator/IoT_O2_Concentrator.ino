#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"   // central config file
// TODO: uncomment GSM include when SIM800L EVB is wired
// #include <TinyGsmClient.h>

// LCD setup
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 20, 4);

// === Shared Data Struct ===
struct SensorData {
  float o2;
  float flow;
  float temp;
  unsigned long uptime;
};

SensorData sharedData;
SemaphoreHandle_t dataMutex;

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
  digitalWrite(RELAY_PIN, LOW); // compressor OFF by default
  digitalWrite(BUZZER_PIN, LOW);

  // LCD init
  lcd.init();
  lcd.backlight();

  // Mutex init
  dataMutex = xSemaphoreCreateMutex();

  // Create tasks
  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, NULL, 1, &sensorTaskHandle, 1);
  xTaskCreatePinnedToCore(displayTask, "DisplayTask", 4096, NULL, 1, &displayTaskHandle, 1);
  xTaskCreatePinnedToCore(alertTask, "AlertTask", 4096, NULL, 1, &alertTaskHandle, 1);
}

// === Sensor Task (demo mode with potentiometer) ===
void sensorTask(void *pvParameters) {
  for (;;) {
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
    lcd.print("% FL:");
    lcd.print(display.flow, 1);

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

    lcd.setCursor(0, 3);
    lcd.print("GSM: -- SIM: --  "); // Placeholder

    vTaskDelay(pdMS_TO_TICKS(DISPLAY_INTERVAL_MS));
  }
}

// === Alert Task ===
void alertTask(void *pvParameters) {
  static unsigned long lastBuzz = 0;
  for (;;) {
    SensorData alert;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      alert = sharedData;
      xSemaphoreGive(dataMutex);
    }

    if (alert.o2 > O2_NORMAL_MIN) {
      digitalWrite(RELAY_PIN, HIGH); // compressor ON
      digitalWrite(BUZZER_PIN, LOW);
    } else if (alert.o2 > O2_WARNING_MIN) {
      digitalWrite(RELAY_PIN, HIGH); // compressor ON
      // WARNING: short intermittent beeps
      if (millis() - lastBuzz > BUZZER_WARNING_GAP) {
        digitalWrite(BUZZER_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(BUZZER_WARNING_MS));
        digitalWrite(BUZZER_PIN, LOW);
        lastBuzz = millis();
      }
      // TODO: uncomment GSM SMS when SIM800L EVB is wired
      // modem.sendSMS(CAREGIVER_NUMBER, "Warning: O2 low");
    } else {
      digitalWrite(RELAY_PIN, LOW);  // compressor OFF
      // DANGER: buzzer ON continuously
      digitalWrite(BUZZER_PIN, HIGH);
      // TODO: uncomment GSM call when SIM800L EVB is wired
      // modem.callNumber(CAREGIVER_NUMBER);
    }

    vTaskDelay(pdMS_TO_TICKS(ALERT_INTERVAL_MS));
  }
}

void loop() {
  // Empty — everything runs in tasks
}
