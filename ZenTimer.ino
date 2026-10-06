#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include "TimerCore.h"
#include "SerialConsole.h"
#include "TimerActions.h"
#include "TimerDisplay.h"
#include "St7789SoftwareSpi.h"
#include "TouchInput.h"
#include "hardware/BoardConfig.h"

#ifndef ZENTIMER_COLOR_TEST
#define ZENTIMER_COLOR_TEST 0
#endif

class ArduinoTimeSource : public TimeSource {
 public:
  uint32_t nowMs() override { return millis(); }
};
ArduinoTimeSource clockSource;
TimerCore timer(clockSource);
SerialConsole console(timer);
ScreenGeometry geometry(BoardConfig::Rotation);
St7789SoftwareSpi lcd;
TimerDisplay display(lcd, geometry);
TouchInput touch(geometry);
bool touchReady = false;
bool wasSerialConnected = false;
#if ZENTIMER_COLOR_TEST
const uint16_t testColors[] = {0xF800,0x07E0,0x001F,0xFFFF,0x0000};
uint8_t testIndex = 0;
bool colorHolding = false;
uint32_t colorSince = 0;
#endif

void setup() {
  Serial.begin(115200);
  timer.setDuration(120); // demo default; TimerCore itself keeps its original default
  lcd.begin();
  touchReady = touch.begin();
#if ZENTIMER_COLOR_TEST
  display.showColor(testColors[0]);
#endif
}
void loop() {
  const uint32_t now = millis();
  timer.update();
  console.update(now);
  const bool connected = bool(Serial);
  if (connected && !wasSerialConnected) {
    Serial.println("LCD: Software-SPI, 280x240 logisch, Y-Offset 20.");
    Serial.println(touchReady ? "Touch 0x15: konfiguriert (IRQ D0, RST D1)." :
                               "Touch nicht bereit; Serial bleibt nutzbar.");
  }
  wasSerialConnected = connected;
  const InputAction action = touch.poll(now);
#if ZENTIMER_COLOR_TEST
  (void)action;
  display.pump();
  if (!display.busy()) {
    if (!colorHolding) { colorHolding = true; colorSince = millis(); }
    if (uint32_t(millis()-colorSince) >= 1000) {
      testIndex = (testIndex+1)%5;
      display.showColor(testColors[testIndex]);
      colorHolding = false;
    }
  }
#else
  if (action != InputAction::None) {
    applyTimerAction(timer,action);
    if (connected) {
      const auto point = touch.lastPosition();
      Serial.print(action == InputAction::Tap ? "Tap: x=" :
                   action == InputAction::SwipeUp ? "Swipe up: x=" : "Swipe down: x="); Serial.print(point.x);
      Serial.print(" y="); Serial.println(point.y);
    }
  }
  display.update(timer,now);
#endif
}
