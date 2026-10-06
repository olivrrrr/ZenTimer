#pragma once
#include "InputAction.h"
#include "ScreenGeometry.h"

// Delay single taps only in a cancellable session. No Arduino / timer dependency.
class TapSequence {
 public:
  void setContext(uint8_t context, bool cancellable) {
    if (context != context_ || !cancellable) pending_ = false;
    context_ = context; cancellable_ = cancellable;
  }
  InputAction tap(uint32_t now, ScreenGeometry::Point position) {
    if (!cancellable_) return InputAction::Tap;
    if (pending_) {
      const int dx = position.x-position_.x, dy = position.y-position_.y;
      if (uint32_t(now-firstTap_) <= WindowMs && dx*dx+dy*dy <= 60*60) {
        pending_ = false;
        return InputAction::DoubleTap;
      }
      // Separate taps: release the earlier single and wait on the new one.
      firstTap_ = now; position_ = position;
      return InputAction::Tap;
    }
    pending_ = true; firstTap_ = now; position_ = position;
    return InputAction::None;
  }
  InputAction flush(uint32_t now) {
    if (pending_ && uint32_t(now-firstTap_) > WindowMs) {
      pending_ = false;
      return InputAction::Tap;
    }
    return InputAction::None;
  }
 private:
  static constexpr uint32_t WindowMs = 400;
  uint32_t firstTap_ = 0;
  ScreenGeometry::Point position_ = {0,0};
  uint8_t context_ = 255;
  bool pending_ = false, cancellable_ = false;
};
