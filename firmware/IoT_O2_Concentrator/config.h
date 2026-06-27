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

// === GPIO Pin Assignments ===
#define RELAY_PIN             26   // Relay control GPIO
#define BUZZER_PIN            27   // Buzzer GPIO
#define POT_PIN               34   // ADC input for demo potentiometer
#define LCD_I2C_ADDR          0x27 // Default I2C address (change to 0x3F if needed)

// === Thresholds ===
extern float O2_NORMAL_MIN;           // Above this = NORMAL
extern float O2_WARNING_MIN;          // Between 70–85 = WARNING, Below this = DANGER

// === Timing ===
#define SENSOR_INTERVAL_MS    500  // Sensor update interval
#define DISPLAY_INTERVAL_MS   500  // LCD refresh interval
#define ALERT_INTERVAL_MS     1000 // Alert task loop interval
#define BUZZER_WARNING_MS     200  // Warning beep duration
#define BUZZER_WARNING_GAP    1000 // Gap between warning beeps

// === GSM Settings ===
// Uncomment TinyGSM include in main sketch when SIM800L EVB is wired
#define GSM_BAUD              9600
extern String CAREGIVER_NUM;   // Hard-coded caregiver number (demo)

// === O2 Settings ===
#define O2_BAUD               9600
 
// === Demo Mode Mapping ===
// Potentiometer ADC range mapped to O2 percentage
#define O2_MIN_PERCENT        21.0
#define O2_MAX_PERCENT        95.6
#define ADC_MAX_VALUE         4095.0

#endif
