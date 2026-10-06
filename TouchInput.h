#pragma once
#include "TouchGesture.h"
#include "TapSequence.h"
#include "TimerCore.h"
class TouchInput {
 public:
  explicit TouchInput(const ScreenGeometry& geometry) : geometry_(geometry), gesture_(geometry) {}
  bool begin();
  InputAction poll(uint32_t now, TimerCore::State state, bool menuOpen=false);
  ScreenGeometry::Point lastPosition() const { return gesture_.position(); }
 private:
  const ScreenGeometry& geometry_;
  TouchGesture gesture_;
  TapSequence taps_;
  uint32_t lastRead_ = 0;
};
