#import <Cocoa/Cocoa.h>
#include "../TimerCore.h"
#include "SimulatorClock.h"
#include <cassert>
#include <cstdio>
#include <cstring>

static NSString* stateTitle(TimerCore::State state) {
  switch (state) {
    case TimerCore::State::Ready: return @"Bereit";
    case TimerCore::State::Running: return @"Läuft";
    case TimerCore::State::Paused: return @"Pausiert";
    case TimerCore::State::Finished: return @"Beendet";
  }
  return @"";
}

// This view only renders a snapshot from the shared C++ core.
// A later stone image can be drawn before the track without changing the core.
@interface TimerView : NSView
@property(nonatomic, assign) TimerCore* timer;
@property(nonatomic) BOOL showRemaining;
@end

@implementation TimerView
- (BOOL)isOpaque { return NO; }
- (void)drawRect:(NSRect)dirtyRect {
  (void)dirtyRect;
  [[NSColor colorWithCalibratedWhite:0.96 alpha:1] setFill];
  NSRectFill(self.bounds);
  if (!self.timer) return;
  NSPoint center = NSMakePoint(NSMidX(self.bounds), NSMidY(self.bounds));
  const CGFloat radius = MIN(NSWidth(self.bounds), NSHeight(self.bounds)) / 2 - 28;
  NSBezierPath* track = [NSBezierPath bezierPathWithOvalInRect:
      NSMakeRect(center.x - radius, center.y - radius, radius * 2, radius * 2)];
  track.lineWidth = 9;
  [[NSColor colorWithCalibratedWhite:0.86 alpha:1] setStroke];
  [track stroke];
  const double progress = 1.0 - double(self.timer->remainingMilliseconds()) /
      self.timer->durationMilliseconds();
  if (progress > 0) {
    NSBezierPath* arc = [NSBezierPath bezierPath];
    arc.lineWidth = 9;
    arc.lineCapStyle = NSLineCapStyleRound;
    // AppKit's unflipped coordinates: 90 degrees is at the top.
    [arc appendBezierPathWithArcWithCenter:center radius:radius startAngle:90
        endAngle:90 - 360 * progress clockwise:YES];
    [[NSColor colorWithCalibratedRed:0.27 green:0.43 blue:0.40 alpha:1] setStroke];
    [arc stroke];
  }
  if (self.showRemaining) {
    const uint32_t seconds = self.timer->remainingSeconds();
    NSString* text = [NSString stringWithFormat:@"%02u:%02u", seconds / 60, seconds % 60];
    NSDictionary* attrs = @{
      NSFontAttributeName: [NSFont monospacedDigitSystemFontOfSize:38 weight:NSFontWeightLight],
      NSForegroundColorAttributeName: [NSColor colorWithCalibratedWhite:0.25 alpha:1]
    };
    NSSize size = [text sizeWithAttributes:attrs];
    [text drawAtPoint:NSMakePoint(center.x - size.width / 2, center.y - size.height / 2)
        withAttributes:attrs];
  }
}
@end

struct SimulatorState {
  SimulatorClock clock;
  TimerCore timer;
  SimulatorState() : timer(clock) {}
};

@interface SimulatorDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate> {
  SimulatorState state_;
  NSWindow* window_;
  TimerView* ring_;
  NSTextField* duration_;
  NSTextField* status_;
  NSTextField* message_;
  NSButton* apply_;
  NSButton* start_;
  NSButton* pause_;
  NSButton* resume_;
  NSButton* cancel_;
  NSButton* remaining_;
  NSPopUpButton* speed_;
  NSTimer* refresh_;
}
- (void)buildWindow;
- (void)refresh;
- (void)selfTest;
@end

@implementation SimulatorDelegate
- (NSButton*)button:(NSString*)title rect:(NSRect)rect action:(SEL)action {
  NSButton* button = [NSButton buttonWithTitle:title target:self action:action];
  button.frame = rect;
  button.bezelStyle = NSBezelStyleRounded;
  [window_.contentView addSubview:button];
  return button;
}
- (NSTextField*)label:(NSString*)text rect:(NSRect)rect {
  NSTextField* label = [NSTextField labelWithString:text];
  label.frame = rect;
  label.alignment = NSTextAlignmentCenter;
  [window_.contentView addSubview:label];
  return label;
}
- (void)buildWindow {
  window_ = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 540, 620)
      styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable
      backing:NSBackingStoreBuffered defer:NO];
  window_.title = @"ZenTimer · Simulator";
  window_.delegate = self;
  window_.releasedWhenClosed = NO;
  window_.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
  window_.contentView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 540, 620)];
  window_.backgroundColor = [NSColor colorWithCalibratedWhite:0.96 alpha:1];
  [self label:@"ZenTimer" rect:NSMakeRect(20, 567, 500, 30)].font =
      [NSFont systemFontOfSize:24 weight:NSFontWeightLight];
  status_ = [self label:@"Bereit" rect:NSMakeRect(20, 535, 500, 24)];
  ring_ = [[TimerView alloc] initWithFrame:NSMakeRect(110, 215, 320, 320)];
  ring_.timer = &state_.timer;
  ring_.showRemaining = YES;
  [window_.contentView addSubview:ring_];

  remaining_ = [NSButton checkboxWithTitle:@"Restzeit anzeigen" target:self action:@selector(toggleRemaining:)];
  remaining_.frame = NSMakeRect(180, 190, 200, 24);
  remaining_.state = NSControlStateValueOn;
  [window_.contentView addSubview:remaining_];
  [self label:@"Dauer (Sekunden)" rect:NSMakeRect(40, 149, 135, 24)];
  duration_ = [[NSTextField alloc] initWithFrame:NSMakeRect(185, 148, 100, 26)];
  duration_.stringValue = @"600";
  duration_.target = self;
  duration_.action = @selector(applyDuration:);
  [window_.contentView addSubview:duration_];
  apply_ = [self button:@"Übernehmen" rect:NSMakeRect(300, 145, 180, 32) action:@selector(applyDuration:)];
  start_ = [self button:@"Start" rect:NSMakeRect(30, 101, 110, 32) action:@selector(startTimer:)];
  pause_ = [self button:@"Pause" rect:NSMakeRect(150, 101, 110, 32) action:@selector(pauseTimer:)];
  resume_ = [self button:@"Fortsetzen" rect:NSMakeRect(270, 101, 110, 32) action:@selector(resumeTimer:)];
  cancel_ = [self button:@"Abbrechen" rect:NSMakeRect(390, 101, 120, 32) action:@selector(cancelTimer:)];
  [self label:@"Zeitablauf" rect:NSMakeRect(105, 57, 110, 24)];
  speed_ = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(230, 55, 190, 28) pullsDown:NO];
  [speed_ addItemsWithTitles:@[@"1× (Echtzeit)", @"10×", @"60×"]];
  speed_.target = self;
  speed_.action = @selector(changeSpeed:);
  [window_.contentView addSubview:speed_];
  message_ = [self label:@"Dauer nur in Bereit/Beendet ändern · 1–86400 s" rect:NSMakeRect(20, 17, 500, 25)];
  message_.font = [NSFont systemFontOfSize:12];
  [self refresh];
  [window_ center];
  [window_ makeKeyAndOrderFront:nil];
}
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
  (void)notification;
  [self buildWindow];
  refresh_ = [NSTimer timerWithTimeInterval:1.0 / 30 target:self
      selector:@selector(tick:) userInfo:nil repeats:YES];
  [[NSRunLoop mainRunLoop] addTimer:refresh_ forMode:NSRunLoopCommonModes];
  [NSApp activateIgnoringOtherApps:YES];
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
  (void)sender;
  return YES;
}
- (void)applicationWillTerminate:(NSNotification*)notification {
  (void)notification;
  [refresh_ invalidate];
}
- (void)tick:(NSTimer*)sender { (void)sender; [self refresh]; }
- (void)refresh {
  state_.timer.update();
  const auto state = state_.timer.state();
  status_.stringValue = [NSString stringWithFormat:@"%@ · Dauer %u s", stateTitle(state), state_.timer.durationSeconds()];
  const BOOL editable = state == TimerCore::State::Ready || state == TimerCore::State::Finished;
  duration_.enabled = editable;
  apply_.enabled = editable;
  start_.enabled = editable;
  pause_.enabled = state == TimerCore::State::Running;
  resume_.enabled = state == TimerCore::State::Paused;
  ring_.needsDisplay = YES;
}
- (void)applyDuration:(id)sender {
  (void)sender;
  [self commitDuration];
  [self refresh];
}
- (BOOL)commitDuration {
  const char* input = [duration_.stringValue stringByTrimmingCharactersInSet:
      NSCharacterSet.whitespaceAndNewlineCharacterSet].UTF8String;
  uint32_t seconds = 0;
  bool valid = input && *input;
  for (const char* p = input; valid && *p; ++p) {
    if (*p < '0' || *p > '9') { valid = false; break; }
    seconds = seconds * 10 + (*p - '0');
    if (seconds > TimerCore::MaxDurationSeconds) valid = false;
  }
  const BOOL accepted = valid && state_.timer.setDuration(seconds);
  message_.stringValue = accepted ? @"Dauer übernommen." :
      @"Ganze Sekunden von 1 bis 86400 eingeben; zuerst abbrechen.";
  return accepted;
}
- (void)startTimer:(id)sender {
  (void)sender;
  // Commit the visible input before starting so an unconfirmed edit is not ignored.
  if ([self commitDuration]) {
    state_.timer.start();
    message_.stringValue = @"Sitzung gestartet.";
  }
  [self refresh];
}
- (void)pauseTimer:(id)sender {
  (void)sender;
  state_.timer.pause();
  [self refresh];
}
- (void)resumeTimer:(id)sender {
  (void)sender;
  state_.timer.resume();
  [self refresh];
}
- (void)cancelTimer:(id)sender {
  (void)sender;
  state_.timer.cancel();
  message_.stringValue = @"Sitzung abgebrochen; Dauer bleibt erhalten.";
  [self refresh];
}
- (void)toggleRemaining:(id)sender {
  (void)sender;
  ring_.showRemaining = remaining_.state == NSControlStateValueOn;
  ring_.needsDisplay = YES;
}
- (void)changeSpeed:(id)sender {
  (void)sender;
  state_.timer.update(); // account for time at the previous speed first
  const double speeds[] = {1, 10, 60};
  state_.clock.setSpeed(speeds[speed_.indexOfSelectedItem]);
  [self refresh];
}
- (void)selfTest {
  [self buildWindow];
  duration_.stringValue = @"0";
  [start_ performClick:nil];
  assert(state_.timer.state() == TimerCore::State::Ready);
  duration_.stringValue = @"120";
  [start_ performClick:nil];
  assert(state_.timer.state() == TimerCore::State::Running);
  assert(!duration_.enabled && pause_.enabled && !resume_.enabled);
  [pause_ performClick:nil];
  assert(state_.timer.state() == TimerCore::State::Paused && resume_.enabled);
  [remaining_ performClick:nil];
  assert(!ring_.showRemaining);
  [remaining_ performClick:nil];
  assert(ring_.showRemaining);
  [speed_ selectItemAtIndex:2];
  [self changeSpeed:nil];
  assert(state_.clock.speed() == 60);
  [resume_ performClick:nil];
  assert(state_.timer.state() == TimerCore::State::Running);
  [cancel_ performClick:nil];
  assert(state_.timer.state() == TimerCore::State::Ready && state_.timer.remainingSeconds() == 120);
  // Render an exact half-time snapshot using the same core with a manual clock.
  class ManualClock : public TimeSource {
   public: uint32_t now = 0; uint32_t nowMs() override { return now; }
  } manual;
  TimerCore snapshot(manual);
  snapshot.setDuration(120); snapshot.start(); manual.now = 60000; snapshot.pause();
  ring_.timer = &snapshot;
  status_.stringValue = @"Pausiert · Dauer 120 s";
  duration_.enabled = NO; apply_.enabled = NO; start_.enabled = NO;
  pause_.enabled = NO; resume_.enabled = YES;
  message_.stringValue = @"Sitzung pausiert · Zeitablauf 60×";
  ring_.needsDisplay = YES;
  // Render the ring directly into a bitmap, independent of window/layer caches.
  NSBitmapImageRep* bitmap = [[NSBitmapImageRep alloc]
      initWithBitmapDataPlanes:nullptr pixelsWide:320 pixelsHigh:320 bitsPerSample:8
      samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
      bytesPerRow:0 bitsPerPixel:0];
  NSGraphicsContext* context = [NSGraphicsContext graphicsContextWithBitmapImageRep:bitmap];
  [NSGraphicsContext saveGraphicsState];
  [NSGraphicsContext setCurrentContext:context];
  [ring_ drawRect:ring_.bounds];
  [NSGraphicsContext restoreGraphicsState];
  NSData* png = [bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
  assert(png && [png writeToFile:@"build/simulator/preview.png" atomically:YES]);
  manual.now = 120000; snapshot.resume(); manual.now = 180000; snapshot.update();
  assert(snapshot.state() == TimerCore::State::Finished);
  [ring_ display]; // exercise rendering the full circle too
  ring_.timer = &state_.timer;
  std::puts("Simulator UI self-test passed; preview: build/simulator/preview.png");
}
@end

int main(int argc, const char* argv[]) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    SimulatorDelegate* delegate = [[SimulatorDelegate alloc] init];
    NSApp.delegate = delegate;
    NSMenu* menu = [[NSMenu alloc] init];
    NSMenuItem* appItem = [[NSMenuItem alloc] init];
    [menu addItem:appItem];
    NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@"ZenTimer"];
    [appMenu addItemWithTitle:@"ZenTimer beenden" action:@selector(terminate:) keyEquivalent:@"q"];
    appItem.submenu = appMenu;
    NSApp.mainMenu = menu;
    if (argc == 2 && !std::strcmp(argv[1], "--self-test")) [delegate selfTest];
    else [NSApp run];
  }
  return 0;
}
