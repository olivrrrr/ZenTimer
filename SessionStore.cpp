#include "SessionStore.h"
#include <stdlib.h>
#include <string.h>
namespace {
const char* outcome(uint8_t flags) { return flags==1 ? "completed" : flags==2 ? "aborted" : "interrupted"; }
void print64(uint64_t value) { char buffer[24]; snprintf(buffer,sizeof(buffer),"%llu",static_cast<unsigned long long>(value)); Serial.print(buffer); }
}
void SessionStore::begin(uint32_t now) {
  lastNow_=now; uptime_=now;
  if (!flash_.begin() || !journal_.mount()) return;
  JournalRecord r;
  if (journal_.latest(SessionJournal::Settings,r)) {
    if (r.duration>=1 && r.duration<=TimerCore::MaxDurationSeconds) timer_.setDuration(r.duration);
    preferences.time=r.preferences&1; preferences.stones=r.preferences&2;
    preferences.brightness=r.brightness>=20 && r.brightness<=100 ? r.brightness : 70;
  }
  // An unfinished session is closed at its last durable checkpoint, never at guessed uptime.
  JournalRecord begin, end;
  if (journal_.latest(SessionJournal::Begin,begin) &&
      (!journal_.latest(SessionJournal::End,end) || end.session!=begin.sequence)) {
    begin.session=begin.sequence;
    JournalRecord point;
    if (journal_.latest(SessionJournal::Checkpoint,point) && point.session==begin.session) begin=point;
    begin.type=SessionJournal::End; begin.flags=SessionJournal::Interrupted; journal_.append(begin);
  }
  r={}; r.type=SessionJournal::Boot;
  if (journal_.append(r)) boot_=r.sequence;
  priorDuration_=timer_.durationSeconds();
}
const char* SessionStore::statusText() const {
  switch(journal_.status()) {
    case SessionJournal::Status::Ready: return "Bereit";
    case SessionJournal::Status::Full: return "Voll (nur Lesen)";
    case SessionJournal::Status::Foreign: return "Fremddaten - gesperrt";
    case SessionJournal::Status::IoError: return "Schreib-/Lesefehler";
    default: return "Flash nicht verfuegbar";
  }
}
void SessionStore::tick(uint32_t now) {
  uptime_+=uint32_t(now-lastNow_); lastNow_=now;
  if (activeSession_ && timer_.state()==TimerCore::State::Finished) {
    checkpoint(SessionJournal::End,SessionJournal::Completed); activeSession_=false;
  } else if (activeSession_ && timer_.state()==TimerCore::State::Running && uint32_t(now-checkpointAt_)>=60000) {
    checkpoint(SessionJournal::Checkpoint); checkpointAt_=now;
  }
}
void SessionStore::beforeAction(uint32_t now) {
  timer_.update(); tick(now);
  priorState_=timer_.state(); priorDuration_=timer_.durationSeconds();
  priorElapsed_=timer_.durationMilliseconds()-timer_.remainingMilliseconds();
}
void SessionStore::afterAction(uint32_t now) {
  if ((priorState_==TimerCore::State::Ready || priorState_==TimerCore::State::Finished) && timer_.state()==TimerCore::State::Running) {
    active_={}; active_.type=SessionJournal::Begin;
    active_.plannedSeconds=timer_.durationSeconds(); active_.startedUtc=utc(); active_.boot=boot_;
    active_.startLow=uint32_t(uptime_); active_.startHigh=uint32_t(uptime_>>32);
    const bool space=journal_.capacity()-journal_.used()>=128; // reserve an End slot
    if (space && journal_.append(active_)) { active_.session=active_.sequence; activeSession_=true; checkpointAt_=now; }
    else if (Serial) Serial.println("Sitzung laeuft, wird aber nicht gespeichert: Speicherstatus pruefen.");
    // Begin's sequence is its stable ID; following records carry it explicitly.
  } else if (activeSession_ && timer_.state()==TimerCore::State::Ready) {
    active_.elapsedMs=priorElapsed_; active_.type=SessionJournal::End; active_.flags=SessionJournal::Aborted;
    journal_.append(active_); activeSession_=false;
  } else if (activeSession_ && priorState_!=timer_.state()) checkpoint(SessionJournal::Checkpoint);
  if (timer_.durationSeconds()!=priorDuration_) saveSettings();
}
void SessionStore::checkpoint(uint8_t type, uint8_t flags) {
  if (type==SessionJournal::Checkpoint && journal_.capacity()-journal_.used()<=64) return;
  active_.type=type; active_.flags=flags;
  active_.elapsedMs=timer_.durationMilliseconds()-timer_.remainingMilliseconds();
  journal_.append(active_);
}
void SessionStore::saveSettings() {
  if (activeSession_ && journal_.capacity()-journal_.used()<=64) return;
  JournalRecord r={}; r.type=SessionJournal::Settings; r.duration=timer_.durationSeconds();
  r.preferences=(preferences.time?1:0)|(preferences.stones?2:0); r.brightness=preferences.brightness;
  journal_.append(r);
}
void SessionStore::printInfo() {
  Serial.print("ZT_INFO {\"device\":\"");
  char id[20]; snprintf(id,sizeof(id),"%08lx%08lx",static_cast<unsigned long>(NRF_FICR->DEVICEID[0]),static_cast<unsigned long>(NRF_FICR->DEVICEID[1])); Serial.print(id);
  Serial.print("\",\"boot\":"); Serial.print(boot_); Serial.print(",\"uptime_ms\":"); print64(uptime_);
  Serial.print(",\"utc\":"); print64(utc()); Serial.print(",\"sessions\":"); Serial.print(journal_.sessions());
  Serial.print(",\"used_bytes\":"); Serial.print(journal_.used()); Serial.print(",\"capacity_bytes\":"); Serial.print(journal_.capacity());
  Serial.print(",\"storage\":\""); Serial.print(statusText()); Serial.println("\"}");
}
void SessionStore::printRecord(const JournalRecord& r) {
  Serial.print("ZT_RECORD {\"id\":"); Serial.print(r.session); Serial.print(",\"planned_seconds\":"); Serial.print(r.plannedSeconds);
  Serial.print(",\"elapsed_ms\":"); Serial.print(r.elapsedMs); Serial.print(",\"started_utc\":"); print64(r.startedUtc);
  Serial.print(",\"boot\":"); Serial.print(r.boot); Serial.print(",\"start_uptime_ms\":"); print64((uint64_t(r.startHigh)<<32)|r.startLow);
  Serial.print(",\"outcome\":\""); Serial.print(outcome(r.flags)); Serial.println("\"}");
}
void SessionStore::startExport() {
  if (timer_.state()==TimerCore::State::Running || timer_.state()==TimerCore::State::Paused || !journal_.readable()) {
    Serial.println("Export nur im Ruhezustand mit lesbarem Speicher."); return;
  }
  printInfo(); Serial.println("ZT_BEGIN"); exportOffset_=0; exportLimit_=journal_.used(); exporting_=true;
}
void SessionStore::exportNext() {
  if (!exporting_) return;
  if (!Serial) { exporting_=false; return; }
  // Bounded scan per loop; USB exports are requested explicitly.
  for (unsigned i=0; i<8 && exportOffset_<exportLimit_; ++i) {
    JournalRecord r; const bool found=journal_.at(exportOffset_,r); exportOffset_+=64;
    if (found && r.type==SessionJournal::End) { printRecord(r); return; }
  }
  if (exportOffset_>=exportLimit_) { Serial.println("ZT_END"); exporting_=false; }
}
bool SessionStore::command(const char* command, uint32_t now) {
  if (!strcmp(command,"info") || !strcmp(command,"storage")) { tick(now); printInfo(); return true; }
  if (!strcmp(command,"export")) { startExport(); return true; }
  if (!strcmp(command,"sessions")) {
    JournalRecord r;
    for (unsigned i=0; i<10 && journal_.latest(SessionJournal::End,r,i); ++i) printRecord(r);
    printInfo(); return true;
  }
  if (!strncmp(command,"time ",5)) {
    const char* p=command+5; uint64_t value=0; bool ok=*p!=0;
    for (; *p && ok; ++p) { if (*p<'0' || *p>'9' || value>4102444800ULL/10) { ok=false; break; } value=value*10+(*p-'0'); }
    if (!ok || value<1577836800ULL || value>4102444800ULL) Serial.println("time: Unix-Sekunden 2020..2100 erwartet.");
    else { tick(now); utcAnchor_=value; anchorUptime_=uptime_; printInfo(); }
    return true;
  }
  return false;
}
