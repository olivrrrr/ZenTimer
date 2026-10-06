#include <Arduino.h>
#include "St7789SoftwareSpi.h"
#include "hardware/BoardConfig.h"
using namespace BoardConfig;

// Intentionally preserve Displaytest's proven bit order and 1us half cycles.
void St7789SoftwareSpi::spiWrite(uint8_t value) {
  for (int bit = 7; bit >= 0; --bit) {
    digitalWrite(LcdClk, LOW);
    digitalWrite(LcdDin, (value & (1 << bit)) ? HIGH : LOW);
    delayMicroseconds(1);
    digitalWrite(LcdClk, HIGH);
    delayMicroseconds(1);
  }
  digitalWrite(LcdClk, LOW);
}
void St7789SoftwareSpi::command(uint8_t value) {
  digitalWrite(LcdCs, LOW); digitalWrite(LcdDc, LOW);
  spiWrite(value);
  digitalWrite(LcdCs, HIGH);
}
void St7789SoftwareSpi::data(uint8_t value) {
  digitalWrite(LcdCs, LOW); digitalWrite(LcdDc, HIGH);
  spiWrite(value);
  digitalWrite(LcdCs, HIGH);
}
void St7789SoftwareSpi::begin() {
  const uint8_t pins[] = {LcdDin, LcdClk, LcdCs, LcdDc, LcdReset, LcdBacklight};
  for (uint8_t pin : pins) pinMode(pin, OUTPUT);
  digitalWrite(LcdClk, LOW); digitalWrite(LcdDin, LOW);
  digitalWrite(LcdCs, HIGH); digitalWrite(LcdBacklight, LOW);
  digitalWrite(LcdReset, HIGH); delay(50);
  digitalWrite(LcdReset, LOW); delay(100);
  digitalWrite(LcdReset, HIGH); delay(150);
  command(0x01); delay(150);
  command(0x11); delay(150);
  command(0x3A); data(0x55);
  command(0x36); data(0x00); // Orientation is in ScreenGeometry, no new registers.
  command(0x13); delay(10);
  command(0x21); delay(10);
  command(0x29); delay(100);
}
void St7789SoftwareSpi::beginRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  const uint16_t xEnd = x + width - 1;
  const uint16_t yStart = y + 20, yEnd = y + height - 1 + 20;
  command(0x2A); data(x >> 8); data(x); data(xEnd >> 8); data(xEnd);
  command(0x2B); data(yStart >> 8); data(yStart); data(yEnd >> 8); data(yEnd);
  command(0x2C);
  digitalWrite(LcdCs, LOW); digitalWrite(LcdDc, HIGH);
}
void St7789SoftwareSpi::writePixels(const uint16_t* pixels, uint16_t count) {
  while (count--) { spiWrite(*pixels >> 8); spiWrite(*pixels & 0xFF); ++pixels; }
}
void St7789SoftwareSpi::endRegion() { digitalWrite(LcdCs, HIGH); }

void St7789SoftwareSpi::setBacklight(bool enabled) {
  digitalWrite(LcdBacklight, enabled ? HIGH : LOW);
}
