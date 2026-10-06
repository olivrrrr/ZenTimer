#pragma once
#include <stdint.h>

// RGB565 transport only; no timer or touch knowledge.
class DisplayDriver {
 public:
  virtual ~DisplayDriver() = default;
  virtual void beginRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height) = 0;
  virtual void writePixels(const uint16_t* pixels, uint16_t count) = 0;
  virtual void endRegion() = 0;
  virtual void setBacklight(bool enabled) = 0;
};
