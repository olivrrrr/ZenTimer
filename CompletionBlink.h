#pragma once
#include <stdint.h>

// Six 300 ms phases: off/on three times, then steady on. No timer state here.
class CompletionBlink {
 public:
  void start(uint32_t now) { started_ = now; active_ = true; }
  void cancel() { active_ = false; }
  bool level(uint32_t now) {
    if (!active_) return true;
    const uint32_t phase = uint32_t(now-started_) / 300;
    if (phase >= 6) { active_ = false; return true; }
    return (phase & 1) != 0;
  }
 private:
  bool active_ = false;
  uint32_t started_ = 0;
};
