#include "../simulator/SimulatorClock.h"
#include "../TimerCore.h"
#include <cassert>

int main() {
  using Clock = SimulatorClock::Clock;
  using Ms = std::chrono::milliseconds;
  const Clock::time_point origin{};
  SimulatorClock clock(origin);
  assert(clock.sample(origin + Ms(250)) == 250);
  clock.setSpeedAt(10, origin + Ms(250));
  assert(clock.sample(origin + Ms(250)) == 250); // no jump on speed change
  assert(clock.sample(origin + Ms(350)) == 1250);
  clock.setSpeedAt(60, origin + Ms(350));
  assert(clock.sample(origin + Ms(450)) == 7250);
  clock.setSpeedAt(1, origin + Ms(450));
  assert(clock.sample(origin + Ms(451)) == 7251);
  SimulatorClock wrap(origin);
  assert(wrap.sample(origin + Ms(4294967295LL)) == UINT32_MAX);
  assert(wrap.sample(origin + Ms(4294967296LL)) == 0);

  // Feed deterministic accelerated samples into the actual shared core.
  class SampleClock : public TimeSource {
   public:
    uint32_t sample = 0;
    uint32_t nowMs() override { return sample; }
  } source;
  SimulatorClock accelerated(origin);
  accelerated.setSpeedAt(60, origin);
  TimerCore timer(source);
  assert(timer.setDuration(120));
  assert(timer.start());
  source.sample = accelerated.sample(origin + Ms(1000));
  assert(timer.pause());
  assert(timer.remainingMilliseconds() == 60000);
  source.sample = accelerated.sample(origin + Ms(10000));
  timer.update();
  assert(timer.remainingMilliseconds() == 60000);
  assert(timer.resume());
  source.sample = accelerated.sample(origin + Ms(11000));
  timer.update();
  assert(timer.state() == TimerCore::State::Finished);
  assert(timer.remainingMilliseconds() == 0);
}
