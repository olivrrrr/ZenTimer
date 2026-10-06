// Exercise the firmware menu and export its real RGB565 rendering on macOS.
#include "../DeviceMenu.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

TestSerial Serial;
TestFicr ficr;
TestFicr* NRF_FICR=&ficr;
std::vector<uint8_t> testFlash(2*1024*1024,255);
struct Clock : TimeSource { uint32_t now=0; uint32_t nowMs() override { return now; } };
class Panel : public DisplayDriver {
 public:
  uint16_t pixels[240*280]={};
  bool light=false, writing=false;
  uint8_t brightness=0;
  uint16_t x=0,y=0,w=0,h=0;
  uint32_t offset=0;
  void setBacklight(bool value) override { light=value; }
  void setBrightness(uint8_t value) override { brightness=value; }
  void beginRegion(uint16_t px,uint16_t py,uint16_t width,uint16_t height) override {
    assert(!writing && width && height && px+width<=240 && py+height<=280);
    x=px;y=py;w=width;h=height;offset=0;writing=true;
  }
  void writePixels(const uint16_t* data,uint16_t count) override {
    assert(writing && count && count<=96 && offset+count<=uint32_t(w)*h);
    for (unsigned i=0;i<count;++i,++offset) pixels[(y+offset/w)*240+x+offset%w]=data[i];
  }
  void endRegion() override { assert(writing && offset==uint32_t(w)*h); writing=false; }
};
int main(int argc,char** argv) {
  assert(argc==2); const std::string directory=argv[1];
  Clock clock; TimerCore timer(clock); timer.setDuration(1200);
  SessionStore store(timer); store.begin(0);
  ScreenGeometry geometry; Panel panel; TimerDisplay display(panel,geometry);
  DeviceMenu menu(timer,store,display);
  auto render=[&]() {
    menu.render();
    unsigned guard=0;
    while(display.busy()) { assert(++guard<2000); menu.render(); }
    menu.render(); assert(!display.busy() && !panel.writing && panel.light);
  };
  auto capture=[&](const char* name) {
    render(); std::ofstream file(directory+"/"+name+".ppm",std::ios::binary); assert(file);
    file<<"P6\n280 240\n255\n";
    for(int y=0;y<240;++y) for(int x=0;x<280;++x) {
      const auto n=geometry.screenToNative(x,y); const uint16_t p=panel.pixels[n.y*240+n.x];
      assert(p==display.pixel(x,y)); // transferred pixels, including orientation
      const char rgb[]={char(((p>>11)&31)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};
      file.write(rgb,3);
    }
  };
  auto tap=[&](int x,int y) { menu.tap(x,y,clock.now); render(); };
  auto root=[&]() { menu.show(); render(); assert(menu.open()); };
  root(); capture("01-main");
  tap(140,60); capture("02-profiles-1");
  tap(240,220); capture("03-profiles-2");
  tap(140,60); assert(!menu.open() && timer.durationSeconds()==2700); // 45 min
  JournalRecord config; assert(store.journal().latest(SessionJournal::Settings,config) && config.duration==2700);
  root(); tap(140,98); capture("04-display");
  tap(140,60); assert(!store.preferences.time);
  tap(140,98); assert(!store.preferences.stones);
  tap(140,136); assert(store.preferences.brightness==100 && panel.brightness==100);
  assert(store.journal().latest(SessionJournal::Settings,config) && config.preferences==0 && config.brightness==100);
  tap(140,60); tap(140,98); tap(140,136); // time/stones restored, brightness 40
  assert(store.preferences.time && store.preferences.stones && panel.brightness==40);
  tap(140,136); assert(panel.brightness==70);
  tap(140,174); assert(!menu.open());
  root(); tap(140,136); capture("10-empty-history");
  // Fixture records exercise dates, outcomes, pagination and details; no personal data.
  for(unsigned i=0;i<6;++i) {
    JournalRecord r={}; r.type=SessionJournal::End; r.session=101+i;
    r.plannedSeconds=1200; r.elapsedMs=i==5?1200000:180000+i*60000;
    r.flags=i==5?SessionJournal::Completed:i==4?SessionJournal::Aborted:SessionJournal::Interrupted;
    r.startedUtc=i==0?0:1790870400+i*86400; assert(store.journal().append(r));
  }
  root(); tap(140,136); capture("05-history-1");
  tap(140,60); capture("06-detail");
  tap(140,220); assert(menu.open()); // actual current behavior: back goes to Main
  root(); tap(140,136); tap(240,220); capture("09-history-2");
  tap(240,220); capture("11-history-last-bounded");
  auto bytes=[&](const char* name) {
    std::ifstream file(directory+"/"+name+".ppm",std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());
  };
  assert(bytes("09-history-2")==bytes("11-history-last-bounded")); // last page is bounded
  tap(140,98); capture("12-detail-undated");
  root(); tap(140,174); capture("07-data");
  tap(140,98); assert(store.exporting());
  for(unsigned guard=0;store.exporting();++guard) { assert(guard<1000); store.exportNext(); }
  const std::string exported=Serial.output.str();
  assert(exported.find("ZT_BEGIN")!=std::string::npos && exported.find("ZT_END")!=std::string::npos);
  unsigned count=0; for(size_t pos=0;(pos=exported.find("ZT_RECORD",pos))!=std::string::npos;pos+=9) ++count;
  assert(count==6);
  tap(140,136); capture("08-storage");
  tap(140,220); tap(140,220); assert(!menu.open());
  timer.start(); menu.show(); assert(!menu.open()); // cannot open in a session
  std::cout<<"PASS: profiles, preferences/persistence, history/details, pagination, export, session lock and pixel transport.\n";
}
