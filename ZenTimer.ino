#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include "TimerCore.h"
#include "SerialConsole.h"

class ArduinoTimeSource : public TimeSource {
 public:
  uint32_t nowMs() override { return millis(); }
};
ArduinoTimeSource clockSource;
TimerCore timer(clockSource);
SerialConsole console(timer);
void setup() { Serial.begin(115200); }
void loop() {
  const uint32_t now = millis();
  timer.update();
  console.update(now);
}
