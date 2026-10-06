#include "../TouchGesture.h"
#include "../TimerActions.h"
#include "../CompletionBlink.h"
#include "../TapSequence.h"
#include <cassert>
#include <initializer_list>
class ManualClock : public TimeSource {
 public: uint32_t now=0; uint32_t nowMs() override { return now; }
};
InputAction at(TouchGesture& gesture,const ScreenGeometry& geometry,bool down,int x,int y,uint32_t now) {
  const auto native=geometry.screenToNative(x,y);
  return gesture.sample(down,native.x,native.y,now);
}
int main() {
  for(auto rotation:{ScreenGeometry::Rotation::Native,ScreenGeometry::Rotation::Clockwise,
                    ScreenGeometry::Rotation::HalfTurn,ScreenGeometry::Rotation::CounterClockwise}) {
    ScreenGeometry geometry(rotation); TouchGesture gesture(geometry);
    assert(at(gesture,geometry,true,100,160,1000)==InputAction::TouchDown);
    assert(at(gesture,geometry,true,102,80,1100)==InputAction::None);
    assert(at(gesture,geometry,false,0,0,1200)==InputAction::SwipeUp);
    // Duplicate release / finger bounce after a swipe must never start the timer.
    assert(at(gesture,geometry,false,0,0,1210)==InputAction::None);
    assert(at(gesture,geometry,true,102,80,1220)==InputAction::None);
    assert(at(gesture,geometry,false,0,0,1230)==InputAction::None);
    assert(at(gesture,geometry,true,100,70,2000)==InputAction::TouchDown);
    assert(at(gesture,geometry,true,100,140,2100)==InputAction::None);
    assert(at(gesture,geometry,false,0,0,2200)==InputAction::SwipeDown);
    assert(at(gesture,geometry,true,100,100,3000)==InputAction::TouchDown);
    assert(at(gesture,geometry,false,0,0,3100)==InputAction::Tap);
    // Long hold, sideways swipe, and travel away then back are not taps.
    at(gesture,geometry,true,100,100,4000);
    assert(at(gesture,geometry,false,0,0,5000)==InputAction::None);
    at(gesture,geometry,true,60,100,6000);at(gesture,geometry,true,160,100,6100);
    assert(at(gesture,geometry,false,0,0,6200)==InputAction::None);
    at(gesture,geometry,true,100,100,7000);at(gesture,geometry,true,100,150,7100);
    at(gesture,geometry,true,100,100,7200);
    assert(at(gesture,geometry,false,0,0,7300)==InputAction::None);
    at(gesture,geometry,true,100,100,8000);gesture.discard();
    assert(at(gesture,geometry,false,0,0,8100)==InputAction::None);
    gesture.sample(true,240,280,9000);
    assert(gesture.sample(false,0,0,9100)==InputAction::None);
  }
  ScreenGeometry geometry; TouchGesture wrap(geometry);
  at(wrap,geometry,true,100,100,UINT32_MAX-100);
  assert(at(wrap,geometry,false,0,0,100)==InputAction::Tap);
  TouchGesture held(geometry);
  at(held,geometry,true,140,120,UINT32_MAX-500);
  assert(!held.longPress(400));
  assert(held.longPress(500)); // wrap-safe one-second hold
  assert(!held.longPress(600)); // only once
  assert(at(held,geometry,false,0,0,700)==InputAction::None); // release cannot start timer
  at(held,geometry,true,140,120,2000); at(held,geometry,true,170,120,2100);
  assert(!held.longPress(4000));
  ManualClock clock; TimerCore timer(clock); assert(timer.setDuration(120));
  applyTimerAction(timer,InputAction::IncreaseDuration);assert(timer.durationSeconds()==180);
  applyTimerAction(timer,InputAction::DecreaseDuration);assert(timer.durationSeconds()==120);
  for(int i=0;i<100;++i) applyTimerAction(timer,InputAction::DecreaseDuration);
  assert(timer.durationSeconds()==60);
  for(int i=0;i<100;++i) applyTimerAction(timer,InputAction::IncreaseDuration);
  assert(timer.durationSeconds()==3600);
  applyTimerAction(timer,InputAction::Tap);assert(timer.state()==TimerCore::State::Running);
  applyTimerAction(timer,InputAction::DecreaseDuration);assert(timer.durationSeconds()==3600);
  applyTimerAction(timer,InputAction::Tap);assert(timer.state()==TimerCore::State::Paused);
  applyTimerAction(timer,InputAction::DecreaseDuration);assert(timer.durationSeconds()==3600);
  applyTimerAction(timer,InputAction::Tap);clock.now=3600000;timer.update();
  assert(timer.state()==TimerCore::State::Finished);
  applyTimerAction(timer,InputAction::DecreaseDuration);assert(timer.durationSeconds()==3600);
  applyTimerAction(timer,InputAction::Tap);assert(timer.state()==TimerCore::State::Ready);
  TapSequence sequence;
  sequence.setContext(0,false);assert(sequence.tap(0,{100,100})==InputAction::Tap);
  sequence.setContext(1,true);assert(sequence.tap(1000,{100,100})==InputAction::None);
  assert(sequence.tap(1250,{110,100})==InputAction::DoubleTap);
  assert(sequence.flush(2000)==InputAction::None); // no extra single after cancel
  assert(sequence.tap(3000,{100,100})==InputAction::None);
  assert(sequence.flush(3400)==InputAction::None);
  assert(sequence.flush(3401)==InputAction::Tap);
  sequence.tap(4000,{100,100});sequence.setContext(2,true);
  assert(sequence.flush(4500)==InputAction::None); // Serial/state changes cancel waiting taps
  sequence.tap(UINT32_MAX-100,{100,100});
  assert(sequence.tap(100,{100,100})==InputAction::DoubleTap);
  applyTimerAction(timer,InputAction::Tap);assert(timer.state()==TimerCore::State::Running);
  applyTimerAction(timer,InputAction::DoubleTap);assert(timer.state()==TimerCore::State::Ready);
  assert(timer.durationSeconds()==3600);
  applyTimerAction(timer,InputAction::Tap);applyTimerAction(timer,InputAction::Tap);
  assert(timer.state()==TimerCore::State::Paused);
  applyTimerAction(timer,InputAction::DoubleTap);assert(timer.state()==TimerCore::State::Ready);
  CompletionBlink blink;blink.start(UINT32_MAX-100);
  assert(!blink.level(UINT32_MAX-100));assert(blink.level(199));
  assert(!blink.level(499));assert(blink.level(799));assert(!blink.level(1099));
  assert(blink.level(1399));assert(blink.level(1699));assert(blink.level(9999));
  blink.start(10000);assert(!blink.level(10000));blink.cancel();assert(blink.level(10001));
}
