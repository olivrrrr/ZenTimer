#pragma once
#include "TouchGesture.h"
class TouchInput {
 public:
  explicit TouchInput(const ScreenGeometry& geometry) : gesture_(geometry) {}
  bool begin();
  InputAction poll(uint32_t now);
  ScreenGeometry::Point lastPosition() const { return gesture_.position(); }
 private:
  TouchGesture gesture_;
  uint32_t lastRead_ = 0;
};
