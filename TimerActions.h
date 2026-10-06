#pragma once
#include "TimerCore.h"
#include "InputAction.h"

// A later proximity input can invoke the same action without knowing the UI.
inline void activateTimer(TimerCore& timer) {
  timer.update();
  switch (timer.state()) {
    case TimerCore::State::Ready: timer.start(); break;
    case TimerCore::State::Running: timer.pause(); break;
    case TimerCore::State::Paused: timer.resume(); break;
    case TimerCore::State::Finished: timer.cancel(); break;
  }
}
inline void applyTimerAction(TimerCore& timer, InputAction action) {
  if (action == InputAction::Tap) { activateTimer(timer); return; }
  if (timer.state() != TimerCore::State::Ready ||
      (action != InputAction::SwipeUp && action != InputAction::SwipeDown)) return;
  int32_t seconds = static_cast<int32_t>(timer.durationSeconds()) + (action == InputAction::SwipeUp ? 60 : -60);
  if (seconds < 60) seconds = 60;
  if (seconds > 3600) seconds = 3600;
  timer.setDuration(seconds);
}
