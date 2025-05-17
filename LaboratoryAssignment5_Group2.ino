#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

// I2C Configuration for STM32F103 (“Blue Pill”)
// SDA → PB7, SCL → PB6
#define I2C_SDA PB7
#define I2C_SCL PB6

#define LCD_ADDR     0x27    // Common LCD address
#define TSL2561_ADDR 0x39    // Default TSL2561 I2C address

// Light range calibration (adjust to your environment)
#define MIN_LUX  0.0         // 0% light level (dark)
#define MAX_LUX 1000.0       // 100% light level (bright)

// Components
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
Adafruit_TSL2561_Unified tsl(TSL2561_ADDR);

// Moving‑average buffer
const uint8_t SAMPLE_SIZE = 5;
float luxReadings[SAMPLE_SIZE];
uint8_t currentReading = 0;
float luxTotal = 0;

// Timing
unsigned long previousUpdate = 0;
const uint16_t UPDATE_INTERVAL = 500;  // 0.5 s

void configureSensor() {
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
}

float mapPercentage(float lux) {
  lux = constrain(lux, MIN_LUX, MAX_LUX);
  return (lux - MIN_LUX) * 100.0 / (MAX_LUX - MIN_LUX);
}

void setup() {
  Serial.begin(9600);

  // Initialize I2C on the STM32 pins
  Wire.begin(I2C_SDA, I2C_SCL);

  // LCD init
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Light Level:");

  // Sensor init
  if (!tsl.begin()) {
    lcd.clear();
    lcd.print("Sensor Error!");
    while (1);
  }
  configureSensor();

  // Pre‑fill moving‑average buffer
  for (uint8_t i = 0; i < SAMPLE_SIZE; i++) {
    sensors_event_t event;
    tsl.getEvent(&event);
    luxReadings[i] = event.light;
    luxTotal += event.light;
    delay(100);
  }
}

void loop() {
  // Read new lux value
  sensors_event_t event;
  tsl.getEvent(&event);

  // Update circular buffer
  luxTotal -= luxReadings[currentReading];
  luxReadings[currentReading] = event.light;
  luxTotal += event.light;
  currentReading = (currentReading + 1) % SAMPLE_SIZE;

  // Periodic update
  if (millis() - previousUpdate >= UPDATE_INTERVAL) {
    previousUpdate = millis();

    float avgLux   = luxTotal / SAMPLE_SIZE;
    float percent  = mapPercentage(avgLux);

    // Display on LCD
    lcd.setCursor(0, 1);
    lcd.print("     ");        // clear
    lcd.setCursor(0, 1);
    lcd.print(percent, 0);     // integer percent
    lcd.print("%");

    // Serial output
    Serial.print("Light Level: ");
    Serial.print(avgLux);
    Serial.print(" lx  (");
    Serial.print(percent, 0);
    Serial.println("%)");
  }
}