#pragma once
#include "TouchGesture.h"
class TouchInput {
 public:
  explicit TouchInput(const ScreenGeometry& geometry) : geometry_(geometry), gesture_(geometry) {}
  bool begin();
  InputAction poll(uint32_t now, bool ready);
  ScreenGeometry::Point lastPosition() const { return gesture_.position(); }
 private:
  const ScreenGeometry& geometry_;
  TouchGesture gesture_;
  uint32_t lastRead_ = 0;
};
