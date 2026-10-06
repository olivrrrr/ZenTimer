#pragma once
#include "../TimeSource.h"
#include <chrono>
#include <cmath>

// Scaling changes preserve the current virtual time, including fractional ms.
// Only the clock is accelerated; TimerCore owns all session calculations.
class SimulatorClock : public TimeSource {
 public:
  using Clock = std::chrono::steady_clock;
  uint32_t nowMs() override { return sample(Clock::now()); }
  void setSpeed(double speed) { setSpeedAt(speed, Clock::now()); }
  void setSpeedAt(double speed, Clock::time_point now) {
    virtualMs_ = elapsedAt(now);
    anchor_ = now;
    speed_ = speed;
  }
  uint32_t sample(Clock::time_point now) const {
    return static_cast<uint32_t>(std::fmod(elapsedAt(now), 4294967296.0));
  }
  double speed() const { return speed_; }
  explicit SimulatorClock(Clock::time_point now = Clock::now()) : anchor_(now) {}
 private:
  double elapsedAt(Clock::time_point now) const {
    return virtualMs_ + std::chrono::duration<double, std::milli>(now - anchor_).count() * speed_;
  }
  Clock::time_point anchor_;
  double virtualMs_ = 0;
  double speed_ = 1;
};
