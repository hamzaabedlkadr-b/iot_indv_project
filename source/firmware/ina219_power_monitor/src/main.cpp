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

  if (!g_ina219.begin()) {
    Serial.println("# Failed to find INA219 chip");
    while (true) {
      delay(100);
    }
  }

#if INA219_USE_16V_400MA_CALIBRATION
  g_ina219.setCalibration_16V_400mA();
#else
  g_ina219.setCalibration_32V_1A();
#endif

  g_start_ms = millis();
  print_banner();
}

void loop() {
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
