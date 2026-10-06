#include "SerialConsole.h"
#include "SessionStore.h"
#include <string.h>
void SerialConsole::printHelp() {
  Serial.println("ZenTimer: Befehle mit Enter abschliessen (115200 Baud).");
  Serial.println("duration <Sekunden> (1..86400), start, pause, resume, cancel, status, help");
  Serial.println("menu, sessions, storage, info, export, time <Unix-Sekunden>");
}
void SerialConsole::printStatus() {
  const char* state = "bereit";
  switch (timer_.state()) {
    case TimerCore::State::Ready: state = "bereit"; break;
    case TimerCore::State::Running: state = "laeuft"; break;
    case TimerCore::State::Paused: state = "pausiert"; break;
    case TimerCore::State::Finished: state = "beendet"; break;
  }
  Serial.print("Status: "); Serial.print(state);
  Serial.print(" | Dauer: "); Serial.print(timer_.durationSeconds());
  Serial.print(" s | Rest: "); Serial.print(timer_.remainingSeconds());
  Serial.println(" s");
}
void SerialConsole::execute(uint32_t now) {
  line_[length_] = '\0';
  char* command = line_;
  while (*command == ' ' || *command == '\t') ++command;
  char* end = command + strlen(command);
  while (end > command && (end[-1] == ' ' || end[-1] == '\t')) *--end = '\0';
  if (!*command) return;
  bool ok = true;
  if (!strcmp(command, "help")) { printHelp(); return; }
  if (!strcmp(command, "status")) { printStatus(); return; }
  if (!strcmp(command,"menu")) { menuRequested_=true; return; }
  if (store_ && store_->command(command,now)) return;
  if (store_) store_->beforeAction(now);
  if (!strcmp(command, "start")) ok = timer_.start();
  else if (!strcmp(command, "pause")) ok = timer_.pause();
  else if (!strcmp(command, "resume")) ok = timer_.resume();
  else if (!strcmp(command, "cancel")) timer_.cancel();
  else if (!strncmp(command, "duration ", 9)) {
    const char* number = command + 9;
    while (*number == ' ') ++number;
    uint32_t seconds = 0;
    ok = *number != '\0';
    for (; *number && ok; ++number) {
      if (*number < '0' || *number > '9') { ok = false; break; }
      seconds = seconds * 10 + (*number - '0');
      if (seconds > TimerCore::MaxDurationSeconds) ok = false;
    }
    if (ok) ok = timer_.setDuration(seconds);
  } else {
    Serial.println("Unbekannter Befehl. 'help' zeigt die Befehle.");
    return;
  }
  if (store_) store_->afterAction(now);
  if (!ok) Serial.println("Nicht moeglich: Zustand oder Dauer pruefen.");
  printStatus();
  reportedState_ = timer_.state();
  lastReport_ = now;
}
void SerialConsole::update(uint32_t now) {
  if (!Serial) {
    connected_ = false;
    length_ = 0;
    overflow_ = false;
    return;
  }
  if (!connected_) {
    connected_ = true;
    printHelp(); printStatus();
    reportedState_ = timer_.state();
    lastReport_ = now;
  }
  // Limit processing per loop so continuous input cannot starve timer updates.
  for (uint8_t count = 0; count < 64 && Serial.available(); ++count) {
    const char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (overflow_) Serial.println("Befehl zu lang (maximal 63 Zeichen).");
      else execute(now);
      length_ = 0;
      overflow_ = false;
    } else if (c == '\b' || c == 127) {
      if (length_ && !overflow_) --length_;
    } else if (!overflow_) {
      if (length_ < sizeof(line_) - 1) line_[length_++] = c;
      else overflow_ = true;
    }
  }
  if (reportedState_ != timer_.state() ||
      (timer_.state() == TimerCore::State::Running && uint32_t(now - lastReport_) >= 5000)) {
    printStatus();
    reportedState_ = timer_.state();
    lastReport_ = now;
  }
}
