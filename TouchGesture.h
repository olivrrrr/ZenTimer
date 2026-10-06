#pragma once
#include "ScreenGeometry.h"
#include "InputAction.h"

// Contact-down event plus release classification; positions use logical pixels.
class TouchGesture {
 public:
  explicit TouchGesture(const ScreenGeometry& geometry) : geometry_(geometry) {}
  InputAction sample(bool down, int16_t rawX, int16_t rawY, uint32_t now) {
    if (!down) {
      if (!tracking_) return InputAction::None;
      tracking_ = false;
      released_ = now; hasRelease_ = true;
      if (invalid_) return InputAction::None;
      const int dx = last_.x - start_.x, dy = last_.y - start_.y;
      const int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
      if (ay >= 40 && ay >= ax * 3 / 2) return dy < 0 ? InputAction::SwipeUp : InputAction::SwipeDown;
      if (uint32_t(now-started_) <= 500 && maxDistanceSquared_ <= 14*14) return InputAction::Tap;
      return InputAction::None;
    }
    const bool newContact = !tracking_;
    ScreenGeometry::Point point;
    const bool valid = geometry_.nativeToScreen(rawX,rawY,point);
    if (!tracking_) {
      tracking_ = true; invalid_ = !valid || (hasRelease_ && uint32_t(now-released_) < 120); started_ = now; maxDistanceSquared_ = 0;
      if (valid) start_ = last_ = point;
    } else if (!valid) invalid_ = true;
    if (valid && !invalid_) {
      last_ = point;
      const int dx = point.x-start_.x, dy = point.y-start_.y;
      const uint32_t distance = dx*dx + dy*dy;
      if (distance > maxDistanceSquared_) maxDistanceSquared_ = distance;
    }
    return newContact && valid && !invalid_ ? InputAction::TouchDown : InputAction::None;
  }
  void discard() { invalid_ = true; } // failed reads must not synthesize a release
  bool tracking() const { return tracking_; }
  ScreenGeometry::Point position() const { return last_; }
 private:
  const ScreenGeometry& geometry_;
  ScreenGeometry::Point start_ = {0,0}, last_ = {0,0};
  uint32_t started_ = 0, released_ = 0, maxDistanceSquared_ = 0;
  bool tracking_ = false, invalid_ = false, hasRelease_ = false;
};
