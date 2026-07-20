#ifndef CONFIG_H
#define CONFIG_H

#define TINY_GSM_MODEM_SIM800

// === I2C Pin Assignments ===
#define I2C_SDA_PIN           21
#define I2C_SCL_PIN           22

// === UART Pin Assignments ===
#define SIM_TX_PIN            14   // UART1 TX to SIM800L EVB SIM_RXD
#define SIM_RX_PIN            13   // UART1 RX from SIM800L EVB SIM_TXD

#define O2_TX_PIN             17   // UART2 TX to OXYGEN SENSOR_RXD
#define O2_RX_PIN             16   // UART2 RX from OXYGEN SENSOR_TXD

// === GPIO Pin Assignments (corrected) ===
#define RELAY_PIN             27   // Relay control GPIO (was 26)
#define BUZZER_PIN            26   // Buzzer GPIO (was 27)
#define POT_PIN               34   // ADC input for demo potentiometer
#define LCD_I2C_ADDR          0x27 // Default I2C address (change to 0x3F if needed)

// === Thresholds (5-tier alert system) ===
// Runtime values, loaded from NVS in setup() with these as fallback defaults.
// Declared here as extern, defined as float globals in firmware.ino.
extern float O2_NORMAL_MIN;    // >= this           = NORMAL   (silent, relay ON)
extern float O2_WARNING_MIN;   // WARNING_MIN to NORMAL_MIN  = WARNING  (single beep, SMS, relay ON)
extern float O2_DANGER_MIN;    // DANGER_MIN to WARNING_MIN  = DANGER   (double beep, call, relay ON)
extern float O2_SEVERE_MIN;    // SEVERE_MIN to DANGER_MIN   = SEVERE   (continuous tone, call, relay ON)
                                // below SEVERE_MIN (3 consecutive readings) = CRITICAL (continuous tone, call, relay OFF)

// === Timing ===
#define SENSOR_INTERVAL_MS    500    // Sensor update interval
#define DISPLAY_INTERVAL_MS   500    // LCD refresh interval
#define ALERT_INTERVAL_MS     1000   // Alert task loop interval
#define BUZZER_WARNING_MS     200    // Warning beep duration
#define BUZZER_WARNING_GAP    500    // Gap between warning beeps (changed from 1000)
#define ALERT_BOOT_DELAY_MS   30000  // Suppress all alerts (buzzer/SMS/call) for this long after boot

// === GSM Settings ===
#define GSM_BAUD              9600
extern String CAREGIVER_NUM;   // Caregiver number, loaded from NVS with demo default

// === O2 Settings ===
#define O2_BAUD               9600

// === Demo Mode Mapping ===
// Potentiometer ADC range mapped to O2 percentage
#define O2_MIN_PERCENT        21.0
#define O2_MAX_PERCENT        95.6
#define ADC_MAX_VALUE         4095.0

// === WiFi Captive Portal (demo input mode) ===
#define WIFI_AP_SSID          "O2-Controller"
#define WIFI_AP_PASS          "12345678"
#define WIFI_AP_IP            "192.168.4.1"

#endif