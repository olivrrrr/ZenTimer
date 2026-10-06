#include <Arduino.h>
#include "nrfx_spim.h"
#ifndef ZENTIMER_SOFTWARE_SPI
#define ZENTIMER_SOFTWARE_SPI 0
#endif
namespace {
const nrfx_spim_t lcdSpi = NRFX_SPIM_INSTANCE(3);
}
#include "St7789SoftwareSpi.h"
#include "hardware/BoardConfig.h"
using namespace BoardConfig;

// Intentionally preserve Displaytest's proven bit order and 1us half cycles.
void St7789SoftwareSpi::spiWrite(uint8_t value) {
  if (hardware_) {
    const nrfx_spim_xfer_desc_t transfer = NRFX_SPIM_XFER_TX(&value,1);
    nrfx_spim_xfer(&lcdSpi,&transfer,0);
    return;
  }
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
#if !ZENTIMER_SOFTWARE_SPI
  nrfx_spim_config_t config = NRFX_SPIM_DEFAULT_CONFIG(
      static_cast<uint8_t>(g_ADigitalPinMap[LcdClk]),
      static_cast<uint8_t>(g_ADigitalPinMap[LcdDin]),
      NRFX_SPIM_PIN_NOT_USED,NRFX_SPIM_PIN_NOT_USED);
  config.frequency = NRF_SPIM_FREQ_8M;
  config.irq_priority = 3;
  // NULL handler: synchronous EasyDMA; transmit buffers remain valid until return.
  hardware_ = nrfx_spim_init(&lcdSpi,&config,nullptr,nullptr) == NRFX_SUCCESS;
#endif
  // If peripheral initialization failed, retain the proven software transport.
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
  if (hardware_) {
    uint8_t bytes[192];
    while (count) {
      const uint16_t block = count>96 ? 96 : count;
      for (uint16_t i=0; i<block; ++i) {
        bytes[i*2] = pixels[i]>>8; bytes[i*2+1] = pixels[i]&0xFF;
      }
      const nrfx_spim_xfer_desc_t transfer = NRFX_SPIM_XFER_TX(bytes,size_t(block)*2);
      nrfx_spim_xfer(&lcdSpi,&transfer,0);
      pixels += block; count -= block;
    }
  } else {
    while (count--) { spiWrite(*pixels >> 8); spiWrite(*pixels & 0xFF); ++pixels; }
  }
}
void St7789SoftwareSpi::endRegion() { digitalWrite(LcdCs, HIGH); }

void St7789SoftwareSpi::setBacklight(bool enabled) {
  lit_=enabled;
  analogWrite(LcdBacklight, enabled ? uint16_t(brightness_)*255/100 : 0);
}

void St7789SoftwareSpi::setBrightness(uint8_t percent) {
  brightness_=percent>100?100:percent; setBacklight(lit_);
}
