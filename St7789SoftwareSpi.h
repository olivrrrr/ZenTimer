#pragma once
#include "DisplayDriver.h"

class St7789SoftwareSpi : public DisplayDriver {
 public:
  void begin();
  bool hardwareTransport() const { return hardware_; }
  void beginRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height) override;
  void writePixels(const uint16_t* pixels, uint16_t count) override;
  void endRegion() override;
  void setBacklight(bool enabled) override;
 private:
  bool hardware_ = false;
  void spiWrite(uint8_t value);
  void command(uint8_t command);
  void data(uint8_t value);
};
