#pragma once
#include "SessionStore.h"
#include "TimerDisplay.h"
class DeviceMenu {
 public:
  DeviceMenu(TimerCore& timer, SessionStore& store, TimerDisplay& display) : timer_(timer), store_(store), display_(display) {}
  bool open() const { return open_; }
  void show() { if (timer_.state()==TimerCore::State::Ready) { open_=true; page_=Main; index_=0; dirty_=true; } }
  void close() { open_=false; display_.invalidate(); }
  void tap(int16_t x, int16_t y, uint32_t now);
  void render();
 private:
  enum Page { Main, Profiles, Display, History, Detail, Data, Storage };
  TimerCore& timer_;
  SessionStore& store_;
  TimerDisplay& display_;
  Page page_=Main;
  uint32_t index_=0, selected_=0;
  bool open_=false, dirty_=true;
};
