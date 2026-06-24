#ifndef CONFIG_H
#define CONFIG_H

// === Pin Assignments ===
#define RELAY_PIN             26   // Relay control GPIO
#define BUZZER_PIN            27   // Buzzer GPIO
#define POT_PIN               34   // ADC input for demo potentiometer
#define LCD_I2C_ADDR          0x27 // Default I2C address (change to 0x3F if needed)

// === Thresholds ===
#define O2_NORMAL_MIN         85.0  // Above this = NORMAL
#define O2_WARNING_MIN        70.0  // Between 70–85 = WARNING
#define O2_DANGER_THRESHOLD   70.0  // Below this = DANGER

// === Timing ===
#define SENSOR_INTERVAL_MS    500  // Sensor update interval
#define DISPLAY_INTERVAL_MS   500  // LCD refresh interval
#define ALERT_INTERVAL_MS     1000 // Alert task loop interval
#define BUZZER_WARNING_MS     200  // Warning beep duration
#define BUZZER_WARNING_GAP    1000 // Gap between warning beeps

// === GSM Settings ===
// Uncomment TinyGSM include in main sketch when SIM800L EVB is wired
#define GSM_BAUD              9600
#define CAREGIVER_NUMBER      "+2348012345678" // Hard-coded caregiver number (demo)

// === Demo Mode Mapping ===
// Potentiometer ADC range mapped to O2 percentage
#define O2_MIN_PERCENT        21.0
#define O2_MAX_PERCENT        95.6
#define ADC_MAX_VALUE         4095.0

#endif
