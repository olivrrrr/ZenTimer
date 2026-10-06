#pragma once
#include <Arduino.h>
#include "TimerCore.h"
class SessionStore;
class SerialConsole {
 public:
  explicit SerialConsole(TimerCore& timer) : timer_(timer) {}
  void update(uint32_t now);
  void attachStore(SessionStore& store) { store_=&store; }
  bool takeMenuRequest() { bool value=menuRequested_; menuRequested_=false; return value; }
 private:
  void execute(uint32_t now);
  void printStatus();
  void printHelp();
  SessionStore* store_=nullptr;
  bool menuRequested_=false;
  TimerCore& timer_;
  char line_[64] = {};
  uint8_t length_ = 0;
  bool overflow_ = false;
  bool connected_ = false;
  TimerCore::State reportedState_ = TimerCore::State::Ready;
  uint32_t lastReport_ = 0;
};
