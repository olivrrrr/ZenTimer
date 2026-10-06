#include "../TimerCore.h"
#include <assert.h>
#include <stdint.h>
class ManualClock : public TimeSource {
 public:
  uint32_t now = 0;
  uint32_t nowMs() override { return now; }
};
int main() {
  ManualClock clock;
  TimerCore t(clock);
  assert(t.state() == TimerCore::State::Ready);
  assert(!t.setDuration(0)); assert(!t.setDuration(86401));
  clock.now = 0; assert(!t.pause()); clock.now = 0; assert(!t.resume());
  assert(t.setDuration(2)); clock.now = UINT32_MAX - 499; assert(t.start());
  clock.now = 0; assert(!t.start()); assert(!t.setDuration(3));
  clock.now = 0; t.update(); // 500 ms across wrap
  assert(t.remainingSeconds() == 2);
  assert(t.remainingMilliseconds() == 1500);
  clock.now = 500; assert(t.pause()); assert(t.remainingSeconds() == 1);
  clock.now = 100000; t.update(); assert(t.remainingSeconds() == 1);
  assert(!t.setDuration(3)); clock.now = 100000; assert(t.resume());
  clock.now = 100999; t.update(); assert(t.state() == TimerCore::State::Running);
  clock.now = 101000; t.update(); assert(t.state() == TimerCore::State::Finished);
  assert(t.remainingSeconds() == 0); clock.now = 200000; assert(t.start());
  t.cancel(); assert(t.state() == TimerCore::State::Ready);
  assert(t.remainingSeconds() == 2); clock.now = 300000; assert(t.start());
  clock.now = 302000; assert(!t.pause()); // expiration wins
  assert(t.setDuration(86400)); clock.now = 0; assert(t.start());
  clock.now = 86400000; t.update(); assert(t.state() == TimerCore::State::Finished);
}
