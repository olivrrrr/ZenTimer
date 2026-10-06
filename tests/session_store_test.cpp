#include "../SessionStore.h"
#include <assert.h>
TestSerial Serial;
TestFicr ficr;
TestFicr* NRF_FICR=&ficr;
std::vector<uint8_t> testFlash(32768,255);
struct Clock : TimeSource { uint32_t now=0; uint32_t nowMs() override { return now; } };
int main() {
  Clock clock; TimerCore timer(clock); timer.setDuration(3);
  SessionStore store(timer); store.begin(0);
  store.beforeAction(0); assert(timer.start()); store.afterAction(0);
  clock.now=1000; store.beforeAction(clock.now); assert(timer.pause()); store.afterAction(clock.now);
  clock.now=10000; store.beforeAction(clock.now); assert(timer.resume()); store.afterAction(clock.now);
  clock.now=12000; timer.update(); store.tick(clock.now);
  JournalRecord r; assert(store.journal().latest(SessionJournal::End,r));
  assert(r.flags==SessionJournal::Completed && r.elapsedMs==3000);
  // Serial start also permits Finished -> Running. Both runs must be recorded.
  store.beforeAction(clock.now); assert(timer.start()); store.afterAction(clock.now);
  clock.now=12500; store.beforeAction(clock.now); timer.cancel(); store.afterAction(clock.now);
  assert(store.journal().sessions()==2); assert(store.journal().latest(SessionJournal::End,r));
  assert(r.flags==SessionJournal::Aborted && r.elapsedMs==500);
  store.beforeAction(clock.now); timer.setDuration(1200); store.afterAction(clock.now);
  store.preferences.time=false; store.preferences.brightness=40; store.saveSettings();
  store.beforeAction(clock.now); timer.start(); store.afterAction(clock.now);
  clock.now=13000; store.beforeAction(clock.now); timer.pause(); store.afterAction(clock.now);
  // Power loss: new clock, core and store read the durable checkpoint and settings.
  Clock reset; TimerCore restored(reset); SessionStore reboot(restored); reboot.begin(0);
  assert(restored.durationSeconds()==1200); assert(!reboot.preferences.time && reboot.preferences.brightness==40);
  assert(reboot.journal().sessions()==3); assert(reboot.journal().latest(SessionJournal::End,r));
  assert(r.flags==SessionJournal::Interrupted && r.elapsedMs==500);
  assert(r.session>0 && r.boot>0);
  SessionStore again(restored); again.begin(0); assert(again.journal().sessions()==3); // recover once
  testFlash.assign(256,255); Clock nearFullClock; TimerCore nearFullTimer(nearFullClock);
  SessionStore nearFull(nearFullTimer); nearFull.begin(0);
  nearFull.beforeAction(0); nearFullTimer.start(); nearFull.afterAction(0);
  nearFullClock.now=500; nearFull.beforeAction(500); nearFullTimer.pause(); nearFull.afterAction(500);
  assert(nearFull.journal().capacity()-nearFull.journal().used()==64); // checkpoint cannot consume End reserve
  nearFull.beforeAction(500); nearFullTimer.cancel(); nearFull.afterAction(500);
  assert(nearFull.journal().sessions()==1 && nearFull.journal().status()==SessionJournal::Status::Full);
}
