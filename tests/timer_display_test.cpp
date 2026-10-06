#include "../TimerDisplay.h"
#include "../TimerActions.h"
#include <cassert>
#include <fstream>
#include <vector>
class ManualClock : public TimeSource {
 public: uint32_t now = 0; uint32_t nowMs() override { return now; }
};
class RecordingDriver : public DisplayDriver {
 public:
  uint16_t panel[240*280] = {};
  uint16_t x=0,y=0,w=0,h=0;
  uint32_t offset=0, regions=0;
  bool open=false, light=false;
  std::vector<bool> lightWrites;
  void setBacklight(bool enabled) override { light=enabled; lightWrites.push_back(enabled); }
  void beginRegion(uint16_t xx,uint16_t yy,uint16_t ww,uint16_t hh) override {
    assert(!open && ww && hh && xx+ww<=240 && yy+hh<=280);
    x=xx;y=yy;w=ww;h=hh;offset=0;open=true;++regions;
  }
  void writePixels(const uint16_t* pixels,uint16_t count) override {
    assert(open && count && count<=96 && offset+count<=uint32_t(w)*h);
    for (uint16_t i=0;i<count;++i,++offset) panel[(y+offset/w)*240+x+offset%w]=pixels[i];
  }
  void endRegion() override { assert(open && offset==uint32_t(w)*h);open=false; }
};
void drain(TimerDisplay& display) { while(display.busy()) display.pump(); }
void save(const TimerDisplay& display,const ScreenGeometry& geometry,const char* path) {
  std::ofstream file(path,std::ios::binary);
  file<<"P6\n"<<geometry.width()<<" "<<geometry.height()<<"\n255\n";
  for(int y=0;y<geometry.height();++y) for(int x=0;x<geometry.width();++x) {
    const uint16_t p=display.pixel(x,y);
    const char rgb[]={char(((p>>11)&31)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};
    file.write(rgb,3);
  }
}
int main() {
  for(auto rotation:{ScreenGeometry::Rotation::Native,ScreenGeometry::Rotation::Clockwise,
                     ScreenGeometry::Rotation::HalfTurn,ScreenGeometry::Rotation::CounterClockwise}) {
    ScreenGeometry g(rotation); ScreenGeometry::Point screen;
    assert(!g.nativeToScreen(240,0,screen));assert(!g.nativeToScreen(0,280,screen));
    for(int x=0;x<g.width();++x) for(int y=0;y<g.height();++y) {
      const auto native=g.screenToNative(x,y);
      assert(g.nativeToScreen(native.x,native.y,screen));assert(screen.x==x && screen.y==y);
    }
  }
  ScreenGeometry geometry;
  ScreenGeometry::Point corner;
  assert(geometry.nativeToScreen(0,0,corner) && corner.x==279 && corner.y==0);
  ManualClock clock; TimerCore timer(clock); RecordingDriver driver;
  TimerDisplay display(driver,geometry); assert(timer.setDuration(120));
  auto settle = [&]() { display.update(timer,clock.now); while(display.busy()) display.update(timer,clock.now); };
  settle();assert(driver.regions==1 && driver.light);
  save(display,geometry,"/tmp/zentimer-ready.ppm");
  const auto count=driver.regions;display.update(timer,clock.now);assert(driver.regions==count);
  assert(display.pixel(140,224)>0 && display.pixel(140,224)<TimerDisplay::Orange); // dim base ring in Ready
  activateTimer(timer);settle();assert(timer.state()==TimerCore::State::Running);
  clock.now=999;timer.update();settle();assert(timer.remainingSeconds()==120);
  clock.now=1000;timer.update();settle();assert(timer.remainingSeconds()==119);
  clock.now=30000;timer.update();settle();
  assert(display.pixel(140,224)==TimerDisplay::Orange); // bottom, start
  assert(display.pixel(36,120)==TimerDisplay::Orange); // left, quarter-turn
  assert(display.pixel(140,16)>0 && display.pixel(140,16)<TimerDisplay::Orange);
  assert(display.pixel(244,120)>0 && display.pixel(244,120)<TimerDisplay::Orange);
  clock.now=60000;timer.update();settle();
  assert(display.pixel(140,16)==TimerDisplay::Orange); // halfway: left semicircle
  assert(display.pixel(244,120)>0 && display.pixel(244,120)<TimerDisplay::Orange); // right shows only the base ring
  assert(display.pixel(140,208)==0); // no linear bar beneath the time
  for(int y=229;y<240;++y) for(int x=0;x<280;++x) assert(display.pixel(x,y)==0);
  // Pixel transport and framebuffer agree, including rotated region traversal.
  for(int y=0;y<geometry.height();++y) for(int x=0;x<geometry.width();++x) {
    const auto n=geometry.screenToNative(x,y);
    assert(driver.panel[n.y*240+n.x]==display.pixel(x,y));
  }
  save(display,geometry,"/tmp/zentimer-running.ppm");
  activateTimer(timer);settle();assert(timer.state()==TimerCore::State::Paused);
  const auto pausedRegions=driver.regions;clock.now=120000;timer.update();settle();
  assert(timer.remainingSeconds()==60 && driver.regions==pausedRegions);
  activateTimer(timer);clock.now=180000;timer.update();
  // Pending pixels must not cause an early completion blink.
  display.update(timer,clock.now);assert(display.busy() && driver.light);
  settle();assert(timer.state()==TimerCore::State::Finished && timer.remainingSeconds()==0);
  assert(display.pixel(244,120)==TimerDisplay::Orange && !driver.light);
  save(display,geometry,"/tmp/zentimer-finished.ppm");
  const std::vector<uint16_t> finalFrame(driver.panel,driver.panel+240*280);
  const auto finalRegions=driver.regions;
  const auto lightCount=driver.lightWrites.size();
  for(unsigned phase=1;phase<=8;++phase) {
    clock.now=180000+phase*300;display.update(timer,clock.now);
    assert(driver.light==(phase>=6 || (phase&1)));
    assert(driver.regions==finalRegions);
    assert(std::vector<uint16_t>(driver.panel,driver.panel+240*280)==finalFrame);
  }
  assert(driver.lightWrites.size()==lightCount+5); // exactly three OFF/ON pairs, no restart
  activateTimer(timer);settle();assert(timer.state()==TimerCore::State::Ready);
  assert(display.pixel(140,224)>0 && display.pixel(140,224)<TimerDisplay::Orange && driver.light);
  // A second session blinks once again; reset during an OFF phase restores light.
  assert(timer.setDuration(1));activateTimer(timer);settle();clock.now+=1000;timer.update();settle();
  assert(!driver.light);activateTimer(timer);display.update(timer,clock.now);assert(driver.light);settle();
  assert(timer.setDuration(86400));settle(); // longer Serial durations still fit inside the circle
  display.showColor(0xF800);drain(display);for(uint16_t p:driver.panel) assert(p==0xF800);
  settle();assert(display.pixel(0,0)==0);
}
