#include <Arduino.h>
#include <FastLED.h>
#include "flight_control.hpp"

// VL53L0X_ADDRESS           0x29
// MPU6886_ADDRESS           0x68
// BMP280_ADDRESS            0x76

void setup()
{
  // init_copter();
  // delay(100);
  USBSerial.begin(115200);
  delay(2000); // モニタが接続するのを待つ
  USBSerial.println("===== Setup reached! Hello from ESP32-S3! =====");
}

void loop()
{
  USBSerial.println("Looping... sending test message.");
  loop_400Hz();
}
