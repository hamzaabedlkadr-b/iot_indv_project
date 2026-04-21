#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <math.h>

#ifndef INA219_I2C_SDA_PIN
#define INA219_I2C_SDA_PIN 21
#endif

#ifndef INA219_I2C_SCL_PIN
#define INA219_I2C_SCL_PIN 22
#endif

#ifndef INA219_SAMPLE_PERIOD_MS
#define INA219_SAMPLE_PERIOD_MS 100
#endif

#ifndef INA219_SERIAL_BAUD
#define INA219_SERIAL_BAUD 115200
#endif

#ifndef INA219_USE_16V_400MA_CALIBRATION
#define INA219_USE_16V_400MA_CALIBRATION 1
#endif

namespace {
Adafruit_INA219 g_ina219;
uint32_t g_start_ms = 0;
bool g_ina219_ready = false;
uint32_t g_last_init_attempt_ms = 0;

void scan_i2c_bus();
void print_banner();

void apply_calibration() {
#if INA219_USE_16V_400MA_CALIBRATION
  g_ina219.setCalibration_16V_400mA();
#else
  g_ina219.setCalibration_32V_1A();
#endif
}

bool try_initialize_ina219(bool verbose) {
  if (g_ina219.begin()) {
    apply_calibration();
    g_start_ms = millis();
    g_ina219_ready = true;
    print_banner();
    return true;
  }

  if (verbose) {
    Serial.println("# Failed to initialize INA219 via Adafruit driver");
    Serial.printf("# INA219 still not detected | sda=%u scl=%u\n",
                  static_cast<unsigned>(INA219_I2C_SDA_PIN),
                  static_cast<unsigned>(INA219_I2C_SCL_PIN));
    scan_i2c_bus();
  }

  return false;
}

void scan_i2c_bus() {
  bool found_any = false;

  Serial.printf("# Scanning I2C bus on SDA=%u SCL=%u\n",
                static_cast<unsigned>(INA219_I2C_SDA_PIN),
                static_cast<unsigned>(INA219_I2C_SCL_PIN));

  for (uint8_t address = 1; address < 0x7F; ++address) {
    Wire.beginTransmission(address);
    const uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf("# I2C device found at 0x%02X\n", address);
      found_any = true;
    }
  }

  if (!found_any) {
    Serial.println("# No I2C devices found");
  }
}

void print_banner() {
  Serial.printf(
      "# INA219 monitor ready | sda=%u scl=%u sample_period_ms=%u calibration=%s\n",
      static_cast<unsigned>(INA219_I2C_SDA_PIN),
      static_cast<unsigned>(INA219_I2C_SCL_PIN),
      static_cast<unsigned>(INA219_SAMPLE_PERIOD_MS),
#if INA219_USE_16V_400MA_CALIBRATION
      "16V_400mA"
#else
      "32V_1A"
#endif
  );
  Serial.println(
      "elapsed_ms\tbus_voltage_V\tshunt_voltage_mV\tload_voltage_V\tcurrent_mA\tpower_mW");
}
}  // namespace

void setup() {
  Serial.begin(INA219_SERIAL_BAUD);
  delay(250);

  Wire.begin(INA219_I2C_SDA_PIN, INA219_I2C_SCL_PIN);
  delay(250);
  g_last_init_attempt_ms = millis();
  (void)try_initialize_ina219(true);
}

void loop() {
  if (!g_ina219_ready) {
    const uint32_t now_ms = millis();
    if ((now_ms - g_last_init_attempt_ms) >= 1000U) {
      g_last_init_attempt_ms = now_ms;
      (void)try_initialize_ina219(true);
    }
    delay(50);
    return;
  }

  const float shunt_voltage_mV = g_ina219.getShuntVoltage_mV();
  const float bus_voltage_V = g_ina219.getBusVoltage_V();
  const float current_mA = fabsf(g_ina219.getCurrent_mA());
  const float power_mW = fabsf(g_ina219.getPower_mW());
  const float load_voltage_V = bus_voltage_V + (shunt_voltage_mV / 1000.0f);
  const uint32_t elapsed_ms = millis() - g_start_ms;

  Serial.printf("%lu\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\n",
                static_cast<unsigned long>(elapsed_ms),
                static_cast<double>(bus_voltage_V),
                static_cast<double>(shunt_voltage_mV),
                static_cast<double>(load_voltage_V),
                static_cast<double>(current_mA),
                static_cast<double>(power_mW));

  delay(INA219_SAMPLE_PERIOD_MS);
}
