#pragma once
#include <stdint.h>
#include "TimeSource.h"

// Hardware-independent logic. Call update at least once per millis wrap period.
class TimerCore {
 public:
  explicit TimerCore(TimeSource& clock) : clock_(clock) {}
  enum class State { Ready, Running, Paused, Finished };
  static constexpr uint32_t MaxDurationSeconds = 86400;
  bool setDuration(uint32_t seconds) {
    if (state_ == State::Running || state_ == State::Paused ||
        seconds == 0 || seconds > MaxDurationSeconds) return false;
    durationMs_ = seconds * 1000UL;
    cancel();
    return true;
  }
  bool start() {
    if (state_ != State::Ready && state_ != State::Finished) return false;
    remainingMs_ = durationMs_;
    lastTick_ = clock_.nowMs();
    state_ = State::Running;
    return true;
  }
  bool pause() {
    update();
    if (state_ != State::Running) return false;
    state_ = State::Paused;
    return true;
  }
  bool resume() {
    if (state_ != State::Paused) return false;
    lastTick_ = clock_.nowMs();
    state_ = State::Running;
    return true;
  }
  void cancel() { remainingMs_ = durationMs_; state_ = State::Ready; }
  void update() {
    if (state_ != State::Running) return;
    const uint32_t now = clock_.nowMs();
    const uint32_t elapsed = now - lastTick_; // unsigned: wrap-safe
    lastTick_ = now;
    if (elapsed >= remainingMs_) {
      remainingMs_ = 0;
      state_ = State::Finished;
    } else remainingMs_ -= elapsed;
  }
  uint32_t remainingMilliseconds() const { return remainingMs_; }
  uint32_t durationMilliseconds() const { return durationMs_; }
  State state() const { return state_; }
  uint32_t durationSeconds() const { return durationMs_ / 1000UL; }
  uint32_t remainingSeconds() const { return (remainingMs_ + 999UL) / 1000UL; }
 private:
  TimeSource& clock_;
  State state_ = State::Ready;
  uint32_t durationMs_ = 600000;
  uint32_t remainingMs_ = durationMs_;
  uint32_t lastTick_ = 0;
};
