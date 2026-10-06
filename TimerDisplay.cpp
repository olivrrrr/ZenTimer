#include "TimerDisplay.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
namespace {
// Compact 5x7 bitmap font. Columns use bit 0 for the top pixel.
const char glyphs[] = "0123456789: ADEFGHINPRSUY";
const uint8_t columns[][5] = {
 {0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},
 {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},
 {0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
 {0x3c,0x4a,0x49,0x49,0x30},{1,0x71,9,5,3},
 {0x36,0x49,0x49,0x49,0x36},{6,0x49,0x49,0x29,0x1e},
 {0,0x36,0x36,0,0},{0,0,0,0,0},
 {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x41,0x41,0x22,0x1c},
 {0x7f,0x49,0x49,0x49,0x41},{0x7f,9,9,9,1},
 {0x3e,0x41,0x49,0x49,0x7a},{0x7f,8,8,8,0x7f},{0,0x41,0x7f,0x41,0},
 {0x7f,2,4,8,0x7f},{0x7f,9,9,9,6},
 {0x7f,9,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},
 {0x3f,0x40,0x40,0x40,0x3f},{7,8,0x70,8,7}
};
static_assert(sizeof(columns)/sizeof(columns[0]) == sizeof(glyphs)-1, "font mismatch");
}
void TimerDisplay::setPixel(int16_t x, int16_t y, uint16_t color) {
  if (x < 0 || y < 0 || x >= geometry_.width() || y >= geometry_.height()) return;
  const auto native = geometry_.screenToNative(x,y);
  frame_[native.y * 240 + native.x] = color;
}
uint16_t TimerDisplay::pixel(int16_t x, int16_t y) const {
  const auto native = geometry_.screenToNative(x,y);
  return frame_[native.y * 240 + native.x];
}
void TimerDisplay::fill(Rect rect, uint16_t color) {
  for (int16_t y = rect.y; y < rect.y+rect.h; ++y)
    for (int16_t x = rect.x; x < rect.x+rect.w; ++x) setPixel(x,y,color);
}
void TimerDisplay::text(const char* value, int16_t x, int16_t y, uint8_t scale) {
  for (; *value; ++value, x += 6 * scale) {
    const char* match = strchr(glyphs,*value);
    if (!match) continue;
    const auto& bits = columns[match-glyphs];
    for (uint8_t col = 0; col < 5; ++col)
      for (uint8_t row = 0; row < 7; ++row)
        if (bits[col] & (1 << row)) fill({int16_t(x+col*scale),int16_t(y+row*scale),scale,scale},0xFFFF);
  }
}
void TimerDisplay::queue(Rect rect) {
  const auto a = geometry_.screenToNative(rect.x,rect.y);
  const auto b = geometry_.screenToNative(rect.x+rect.w-1,rect.y+rect.h-1);
  const int16_t x = a.x < b.x ? a.x : b.x, y = a.y < b.y ? a.y : b.y;
  jobs_[jobCount_++] = {x,y,int16_t((a.x>b.x?a.x-b.x:b.x-a.x)+1),int16_t((a.y>b.y?a.y-b.y:b.y-a.y)+1)};
}
bool TimerDisplay::circle(uint16_t steps, Rect& changed) {
  const int16_t cx = geometry_.width()/2, cy = geometry_.height()/2;
  const int16_t radius = (geometry_.width()<geometry_.height()?geometry_.width():geometry_.height())/2-14;
  const int inner = radius-4;
  int16_t xMin = geometry_.width(), yMin = geometry_.height(), xMax = -1, yMax = -1;
  constexpr float Tau = 6.28318530718f;
  for (int16_t dy = -radius; dy <= radius; ++dy) {
    for (int16_t dx = -radius; dx <= radius; ++dx) {
      const int distance = dx*dx+dy*dy;
      if (distance < inner*inner || distance > radius*radius) continue;
      // Logical y grows downwards: bottom -> left -> top -> right is clockwise.
      float angle = atan2f(-dx,dy);
      if (angle < 0) angle += Tau;
      const uint16_t color = steps && (steps == 72 || angle <= steps*Tau/72) ? Orange : 0;
      const int16_t x = cx+dx, y = cy+dy;
      if (pixel(x,y) == color) continue;
      setPixel(x,y,color);
      if (x < xMin) xMin = x;
      if (x > xMax) xMax = x;
      if (y < yMin) yMin = y;
      if (y > yMax) yMax = y;
    }
  }
  if (xMax < 0) return false;
  changed = {xMin,yMin,int16_t(xMax-xMin+1),int16_t(yMax-yMin+1)};
  return true;
}
void TimerDisplay::setLight(bool on) {
  if (on == lightOn_) return;
  driver_.setBacklight(on); lightOn_ = on;
}
void TimerDisplay::updateBacklight(const TimerCore& timer, uint32_t now) {
  if (timer.state() != TimerCore::State::Finished) {
    finishedObserved_ = false; completionPending_ = false; blink_.cancel();
  } else if (!finishedObserved_) {
    finishedObserved_ = true; completionPending_ = true;
  }
  // Start only after the final snapshot has actually reached the panel.
  if (completionPending_ && hasFrame_ && !busy() && state_ == TimerCore::State::Finished) {
    blink_.start(now); completionPending_ = false;
  }
  if (hasFrame_) setLight(blink_.level(now));
}
void TimerDisplay::update(const TimerCore& timer, uint32_t now) {
  updateBacklight(timer,now);
  if (busy()) { pump(); updateBacklight(timer,now); return; }
  const uint32_t seconds = timer.remainingSeconds();
  const uint16_t steps = uint64_t(timer.durationMilliseconds()-timer.remainingMilliseconds())*72/timer.durationMilliseconds();
  const bool timeChanged = first_ || seconds != seconds_;
  const bool circleChanged = first_ || steps != circleSteps_;
  if (!timeChanged && !circleChanged) {
    state_ = timer.state(); updateBacklight(timer,now); return;
  }
  jobIndex_ = jobCount_ = 0; offset_ = 0;
  if (first_) for (uint16_t& pixel : frame_) pixel = 0;
  const int16_t width = geometry_.width(), height = geometry_.height();
  const int16_t radius = (width<height?width:height)/2-14;
  const int16_t maxTextWidth = 2*(radius-4-12);
  const Rect timeArea = {int16_t((width-maxTextWidth)/2-2),int16_t(height/2-25),int16_t(maxTextWidth+4),50};
  if (timeChanged) {
    char value[12];
    snprintf(value,sizeof(value),"%02lu:%02lu",static_cast<unsigned long>(seconds/60),static_cast<unsigned long>(seconds%60));
    fill(timeArea,0);
    uint8_t scale = 6;
    while ((strlen(value)*6-1)*scale > uint16_t(maxTextWidth) && scale > 1) --scale;
    text(value,(width-(strlen(value)*6-1)*scale)/2,(height-7*scale)/2,scale);
  }
  Rect arcArea = {};
  const bool arcChanged = circleChanged && circle(steps,arcArea);
  if (first_) queue({0,0,width,height});
  else {
    // Each changed element is a single contiguous write, including its black background.
    const bool arcContainsTime = arcChanged && arcArea.x <= timeArea.x && arcArea.y <= timeArea.y &&
      arcArea.x+arcArea.w >= timeArea.x+timeArea.w && arcArea.y+arcArea.h >= timeArea.y+timeArea.h;
    if (timeChanged && !arcContainsTime) queue(timeArea);
    if (arcChanged) queue(arcArea);
  }
  first_ = false; state_ = timer.state(); seconds_ = seconds; circleSteps_ = steps;
  pump(); updateBacklight(timer,now);
}
void TimerDisplay::pump() {
  if (!busy()) return;
  const Rect region = jobs_[jobIndex_];
  if (offset_ == 0) driver_.beginRegion(region.x,region.y,region.w,region.h);
  uint16_t pixels[96];
  const uint32_t total = uint32_t(region.w)*region.h;
  uint16_t count = 0;
  while (count < 96 && offset_ < total) {
    const uint32_t index = (region.y + offset_/region.w)*240 + region.x + offset_%region.w;
    pixels[count++] = frame_[index]; ++offset_;
  }
  driver_.writePixels(pixels,count);
  if (offset_ == total) {
    driver_.endRegion(); ++jobIndex_; offset_ = 0;
    if (!busy() && !hasFrame_) { hasFrame_ = true; setLight(true); }
  }
}
void TimerDisplay::showColor(uint16_t color) {
  if (busy()) return;
  for (uint16_t& pixel : frame_) pixel = color;
  jobIndex_ = jobCount_ = 0; offset_ = 0;
  queue({0,0,geometry_.width(),geometry_.height()});
  first_ = true;
}
