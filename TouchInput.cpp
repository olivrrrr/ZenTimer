#include <Arduino.h>
#include <Wire.h>
#include "TouchInput.h"
#include "hardware/BoardConfig.h"
namespace {
volatile bool pendingIrq = false;
void touchIsr() { pendingIrq = true; }
constexpr uint8_t Address = 0x15;
}
bool TouchInput::begin() {
  pinMode(BoardConfig::TouchReset, OUTPUT);
  pinMode(BoardConfig::TouchIrq, INPUT_PULLUP);
  digitalWrite(BoardConfig::TouchReset, HIGH); delay(10);
  digitalWrite(BoardConfig::TouchReset, LOW); delay(20);
  digitalWrite(BoardConfig::TouchReset, HIGH); delay(100);
  Wire.begin(); Wire.setClock(400000);
  // CST816S IrqCtl: EnTouch | EnChange. Host recognizes complete contacts.
  Wire.beginTransmission(Address); Wire.write(0xFA); Wire.write(0x60);
  const bool configured = Wire.endTransmission() == 0;
  attachInterrupt(digitalPinToInterrupt(BoardConfig::TouchIrq), touchIsr, FALLING);
  return configured;
}
InputAction TouchInput::poll(uint32_t now, bool ready) {
  if (uint32_t(now-lastRead_) < 15) return InputAction::None;
  noInterrupts(); const bool pending = pendingIrq; pendingIrq = false; interrupts();
  if (!pending && !gesture_.tracking() && digitalRead(BoardConfig::TouchIrq) == HIGH)
    return InputAction::None;
  lastRead_ = now;
  Wire.beginTransmission(Address); Wire.write(0x02);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(Address, size_t(5)) != 5) {
    gesture_.discard();
    return InputAction::None;
  }
  uint8_t values[5];
  for (uint8_t& value : values) value = Wire.read();
  const int16_t rawX = ((values[1] & 0x0F) << 8) | values[2];
  const int16_t rawY = ((values[3] & 0x0F) << 8) | values[4];
  const auto event = gesture_.sample((values[0] & 0x0F) != 0,rawX,rawY,now);
  if (event == InputAction::TouchDown && ready) {
    const auto point = gesture_.position();
    // Full-height side zones: 64 px wide at 280x240; a hold acts only once.
    const int sideWidth = geometry_.width()*8/35;
    if (point.x < sideWidth || point.x >= geometry_.width()-sideWidth) {
      gesture_.discard(); // consume the contact, including its eventual release
      return point.x < sideWidth ? InputAction::DecreaseDuration : InputAction::IncreaseDuration;
    }
  }
  return event == InputAction::Tap ? event : InputAction::None;
}
