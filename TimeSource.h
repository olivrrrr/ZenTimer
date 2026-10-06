#pragma once
#include <stdint.h>

// Monotonic milliseconds modulo 2^32. Sources must outlive their timer.
class TimeSource {
 public:
  virtual ~TimeSource() = default;
  virtual uint32_t nowMs() = 0;
};
