#pragma once
#include <stdint.h>

// Native panel / CST coordinates stay 240x280, MADCTL remains 0x00.
class ScreenGeometry {
 public:
  enum class Rotation { Native, Clockwise, HalfTurn, CounterClockwise };
  struct Point { int16_t x, y; };
  explicit ScreenGeometry(Rotation rotation = Rotation::Clockwise) : rotation_(rotation) {}
  int16_t width() const { return landscape() ? 280 : 240; }
  int16_t height() const { return landscape() ? 240 : 280; }
  bool nativeToScreen(int16_t x, int16_t y, Point& point) const {
    if (x < 0 || x >= 240 || y < 0 || y >= 280) return false;
    switch (rotation_) {
      case Rotation::Native: point = {x, y}; break;
      case Rotation::Clockwise: point = {int16_t(279-y), x}; break;
      case Rotation::HalfTurn: point = {int16_t(239-x), int16_t(279-y)}; break;
      case Rotation::CounterClockwise: point = {y, int16_t(239-x)}; break;
    }
    return true;
  }
  Point screenToNative(int16_t x, int16_t y) const {
    switch (rotation_) {
      case Rotation::Native: return {x, y};
      case Rotation::Clockwise: return {y, int16_t(279-x)};
      case Rotation::HalfTurn: return {int16_t(239-x), int16_t(279-y)};
      case Rotation::CounterClockwise: return {int16_t(239-y), x};
    }
    return {x,y};
  }
 private:
  bool landscape() const { return rotation_ == Rotation::Clockwise || rotation_ == Rotation::CounterClockwise; }
  Rotation rotation_;
};
