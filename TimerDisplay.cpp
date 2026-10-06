#include "TimerDisplay.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
namespace {
constexpr uint16_t CircleSteps = 180; // two-degree increments
constexpr uint16_t ControlGrey = 0x738E;
float clampCoverage(float value) { return value < 0 ? 0 : value > 1 ? 1 : value; }
uint16_t shade(uint16_t color, float coverage) {
  const uint16_t r = ((color>>11)&31)*coverage+0.5f;
  const uint16_t g = ((color>>5)&63)*coverage+0.5f;
  const uint16_t b = (color&31)*coverage+0.5f;
  return (r<<11)|(g<<5)|b;
}
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
void TimerDisplay::stroke(float x0, float y0, float x1, float y1, float thickness, uint16_t color) {
  const float padding = thickness/2+1;
  const int xMin = floorf(fminf(x0,x1)-padding), xMax = ceilf(fmaxf(x0,x1)+padding);
  const int yMin = floorf(fminf(y0,y1)-padding), yMax = ceilf(fmaxf(y0,y1)+padding);
  const float dx = x1-x0, dy = y1-y0, lengthSquared = dx*dx+dy*dy;
  for (int y=yMin; y<=yMax; ++y) for (int x=xMin; x<=xMax; ++x) {
    const float t = lengthSquared ? clampCoverage(((x-x0)*dx+(y-y0)*dy)/lengthSquared) : 0;
    const float px = x-x0-t*dx, py = y-y0-t*dy;
    const float coverage = clampCoverage(thickness/2+0.5f-sqrtf(px*px+py*py));
    if (!coverage || x<0 || y<0 || x>=geometry_.width() || y>=geometry_.height()) continue;
    const uint16_t ink = shade(color,coverage), old = pixel(x,y);
    // Joined strokes retain the stronger coverage instead of darkening their joins.
    if (ink > old) setPixel(x,y,ink);
  }
}
void TimerDisplay::timeText(const char* value, int16_t maxWidth) {
  const uint8_t masks[] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
  const float segments[7][4] = {{5,2,23,2},{26,6,26,22},{26,30,26,46},
      {5,50,23,50},{2,30,2,46},{2,6,2,22},{5,26,23,26}};
  float length = -6;
  for (const char* p=value; *p; ++p) length += (*p==':' ? 8 : 28)+6;
  const float scale = fminf(1.0f,maxWidth/length);
  float x = (geometry_.width()-length*scale)/2;
  const float y = (geometry_.height()-54*scale)/2;
  for (; *value; ++value) {
    if (*value==':') {
      stroke(x+4*scale,y+18*scale,x+4*scale,y+18*scale,4*scale,0xFFFF);
      stroke(x+4*scale,y+36*scale,x+4*scale,y+36*scale,4*scale,0xFFFF);
      x += 14*scale;
    } else {
      if (*value>='0' && *value<='9') for (uint8_t i=0;i<7;++i) {
        if (!(masks[*value-'0'] & (1<<i))) continue;
        stroke(x+segments[i][0]*scale,y+segments[i][1]*scale,
               x+segments[i][2]*scale,y+segments[i][3]*scale,3.2f*scale,0xFFFF);
      }
      x += 34*scale;
    }
  }
}
void TimerDisplay::controls(bool visible) {
  const int16_t width = geometry_.width(), cy = geometry_.height()/2;
  fill({4,int16_t(cy-16),28,32},0);
  fill({int16_t(width-32),int16_t(cy-16),28,32},0);
  if (!visible) return;
  stroke(10,cy,26,cy,2.5f,ControlGrey);
  stroke(width-26,cy,width-10,cy,2.5f,ControlGrey);
  stroke(width-18,cy-8,width-18,cy+8,2.5f,ControlGrey);
}
void TimerDisplay::queue(Rect rect) {
  const auto a = geometry_.screenToNative(rect.x,rect.y);
  const auto b = geometry_.screenToNative(rect.x+rect.w-1,rect.y+rect.h-1);
  const int16_t x = a.x < b.x ? a.x : b.x, y = a.y < b.y ? a.y : b.y;
  jobs_[jobCount_++] = {x,y,int16_t((a.x>b.x?a.x-b.x:b.x-a.x)+1),int16_t((a.y>b.y?a.y-b.y:b.y-a.y)+1)};
}
bool TimerDisplay::circle(uint16_t steps, Rect& changed) {
  const int16_t cx = geometry_.width()/2, cy = geometry_.height()/2;
  const int16_t radius = (geometry_.width()<geometry_.height()?geometry_.width():geometry_.height())/2-16;
  int16_t xMin = geometry_.width(), yMin = geometry_.height(), xMax = -1, yMax = -1;
  constexpr float Tau = 6.28318530718f;
  const float sweep = float(steps)*Tau/CircleSteps;
  const float endX = -radius*sinf(sweep), endY = radius*cosf(sweep);
  for (int16_t dy=-radius-3; dy<=radius+3; ++dy) for (int16_t dx=-radius-3; dx<=radius+3; ++dx) {
    const float distance = sqrtf(float(dx*dx+dy*dy));
    if (fabsf(distance-radius)>3) continue;
    float coverage = 0;
    if (steps) {
      float angle = atan2f(-dx,dy);
      if (angle<0) angle+=Tau;
      if (steps==CircleSteps || angle<=sweep) coverage=clampCoverage(2.0f-fabsf(distance-radius));
      if (steps<CircleSteps) {
        const float startDistance=sqrtf(float(dx*dx+(dy-radius)*(dy-radius)));
        const float endDistance=sqrtf((dx-endX)*(dx-endX)+(dy-endY)*(dy-endY));
        coverage=fmaxf(coverage,clampCoverage(2.0f-fminf(startDistance,endDistance)));
      }
    }
    const uint16_t color=shade(Orange,coverage);
    const int16_t x=cx+dx,y=cy+dy;
    if (pixel(x,y)==color) continue;
    setPixel(x,y,color);
    if (x<xMin) xMin=x;
    if (x>xMax) xMax=x;
    if (y<yMin) yMin=y;
    if (y>yMax) yMax=y;
  }
  if (xMax<0) return false;
  changed={xMin,yMin,int16_t(xMax-xMin+1),int16_t(yMax-yMin+1)};
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
  const uint16_t steps = uint64_t(timer.durationMilliseconds()-timer.remainingMilliseconds())*CircleSteps/timer.durationMilliseconds();
  const bool timeChanged = first_ || seconds != seconds_;
  const bool circleChanged = first_ || steps != circleSteps_;
  const bool controlsChanged = first_ || (timer.state()==TimerCore::State::Ready) != (state_==TimerCore::State::Ready);
  if (!timeChanged && !circleChanged && !controlsChanged) {
    state_ = timer.state(); updateBacklight(timer,now); return;
  }
  jobIndex_ = jobCount_ = 0; offset_ = 0;
  if (first_) for (uint16_t& pixel : frame_) pixel = 0;
  const int16_t width = geometry_.width(), height = geometry_.height();
  const int16_t radius = (width<height?width:height)/2-16;
  const int16_t maxTextWidth = 2*(radius-4-12);
  const Rect timeArea = {int16_t((width-maxTextWidth)/2-2),int16_t(height/2-29),int16_t(maxTextWidth+4),58};
  if (timeChanged) {
    char value[12];
    snprintf(value,sizeof(value),"%02lu:%02lu",static_cast<unsigned long>(seconds/60),static_cast<unsigned long>(seconds%60));
    fill(timeArea,0);
    timeText(value,maxTextWidth);
  }
  if (controlsChanged) controls(timer.state()==TimerCore::State::Ready);
  Rect arcArea = {};
  const bool arcChanged = circleChanged && circle(steps,arcArea);
  if (first_) queue({0,0,width,height});
  else {
    // Each changed element is a single contiguous write, including its black background.
    const bool arcContainsTime = arcChanged && arcArea.x <= timeArea.x && arcArea.y <= timeArea.y &&
      arcArea.x+arcArea.w >= timeArea.x+timeArea.w && arcArea.y+arcArea.h >= timeArea.y+timeArea.h;
    if (timeChanged && !arcContainsTime) queue(timeArea);
    if (arcChanged) queue(arcArea);
    if (controlsChanged) {
      queue({4,int16_t(height/2-16),28,32});
      queue({int16_t(width-32),int16_t(height/2-16),28,32});
    }
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
