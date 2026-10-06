#pragma once
#include "TimerCore.h"
#include "DisplayDriver.h"
#include "ScreenGeometry.h"
#include "CompletionBlink.h"

class TimerDisplay {
 public:
  TimerDisplay(DisplayDriver& driver, const ScreenGeometry& geometry) : driver_(driver), geometry_(geometry) {}
  void update(const TimerCore& timer, uint32_t now);
  void pump();
  void showColor(uint16_t color);
  bool busy() const { return jobIndex_ < jobCount_; }
  uint16_t pixel(int16_t x, int16_t y) const;
  static constexpr uint16_t Orange = 0xFC60;
 private:
  struct Rect { int16_t x, y, w, h; };
  void fill(Rect rect, uint16_t color);
  void text(const char* text, int16_t x, int16_t y, uint8_t scale);
  void queue(Rect rect);
  bool circle(uint16_t steps, Rect& changed);
  void updateBacklight(const TimerCore& timer, uint32_t now);
  void setLight(bool on);
  void setPixel(int16_t x, int16_t y, uint16_t color);
  DisplayDriver& driver_;
  const ScreenGeometry& geometry_;
  uint16_t frame_[240 * 280] = {};
  Rect jobs_[3] = {};
  uint8_t jobIndex_ = 0, jobCount_ = 0;
  uint32_t offset_ = 0;
  bool first_ = true;
  TimerCore::State state_ = TimerCore::State::Ready;
  uint32_t seconds_ = 0;
  uint16_t circleSteps_ = 0;
  bool hasFrame_ = false, lightOn_ = false;
  bool finishedObserved_ = false, completionPending_ = false;
  CompletionBlink blink_;
};
