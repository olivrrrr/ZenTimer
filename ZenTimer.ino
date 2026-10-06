#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);   // LED an
  delay(500);

  digitalWrite(LED_BUILTIN, HIGH);  // LED aus
  delay(500);
}