#pragma once
#include <stdint.h>
#include <string.h>

// Portable append-only NOR journal. No erase operation is exposed deliberately.
class JournalFlash {
 public:
  virtual ~JournalFlash() = default;
  virtual uint32_t size() const = 0;
  virtual bool read(uint32_t address, void* data, uint32_t length) = 0;
  virtual bool write(uint32_t address, const void* data, uint32_t length) = 0;
};
struct JournalRecord {
  uint32_t magic, sequence;
  uint8_t type, flags;
  uint16_t reserved;
  uint32_t session, plannedSeconds, elapsedMs;
  uint64_t startedUtc;
  uint32_t boot, startLow, startHigh;
  uint32_t duration, preferences, brightness;
  uint32_t crc, commit;
};
static_assert(sizeof(JournalRecord) == 64, "Journal format must remain 64 bytes");
class SessionJournal {
 public:
  enum Type : uint8_t { Header, Settings, Boot, Begin, Checkpoint, End };
  enum Outcome : uint8_t { Completed = 1, Aborted = 2, Interrupted = 3 };
  enum class Status { Unavailable, Ready, Full, Foreign, IoError };
  explicit SessionJournal(JournalFlash& flash) : flash_(flash) {}
  static uint32_t checksum(const JournalRecord& record) {
    uint32_t crc = 0xffffffff;
    const auto* bytes = reinterpret_cast<const uint8_t*>(&record);
    for (unsigned i=0; i<56; ++i) {
      crc ^= bytes[i];
      for (unsigned bit=0; bit<8; ++bit) crc=(crc>>1) ^ (0xedb88320u & (0u-(crc&1)));
    }
    return ~crc;
  }
  static bool valid(const JournalRecord& r) {
    return r.magic == Magic && r.commit == Commit && r.type <= End && r.crc == checksum(r);
  }
  bool mount() {
    next_=sequence_=count_=0; status_=Status::Unavailable;
    if (flash_.size()<64 || flash_.size()%64) return false;
    bool header=false;
    for (uint32_t address=0; address<flash_.size(); address+=64) {
      JournalRecord r;
      if (!flash_.read(address,&r,64)) { status_=Status::IoError; return false; }
      bool blank=true;
      for (const uint8_t c : reinterpretBytes(r)) if (c!=255) { blank=false; break; }
      if (blank) continue;
      next_=address+64;
      // Unknown contents are never reformatted. A torn first header also needs manual recovery.
      bool tornMagic=header && (r.magic & Magic)==Magic;
      const auto* raw=reinterpret_cast<const uint8_t*>(&r);
      for (unsigned i=4; i<64 && tornMagic; ++i) if (raw[i]!=255) tornMagic=false;
      if ((r.magic!=Magic && !tornMagic) || (!header && (address!=0 || !valid(r) || r.type!=Header))) {
        status_=Status::Foreign; return false;
      }
      if (!valid(r)) continue; // power loss during append: retain slot, skip record
      if (address==0) header=true;
      if (r.sequence>sequence_) sequence_=r.sequence;
      if (r.type==End) ++count_;
    }
    status_=next_==flash_.size() ? Status::Full : Status::Ready;
    if (!header) { JournalRecord r={}; r.type=Header; return append(r); }
    return true;
  }
  bool append(JournalRecord& r) {
    if (status_!=Status::Ready) return false;
    r.magic=Magic; r.sequence=sequence_+1; r.crc=checksum(r); r.commit=Commit;
    const uint32_t address=next_; next_+=64; // never retry a partially programmed slot
    if (!flash_.write(address,&r,60) || !flash_.write(address+60,&r.commit,4)) {
      status_=Status::IoError; return false;
    }
    JournalRecord verify;
    if (!flash_.read(address,&verify,64) || memcmp(&r,&verify,64) || !valid(verify)) {
      status_=Status::IoError; return false;
    }
    sequence_=r.sequence; if (r.type==End) ++count_;
    if (next_==flash_.size()) status_=Status::Full;
    return true;
  }
  bool at(uint32_t address, JournalRecord& r) {
    return address<next_ && address%64==0 && flash_.read(address,&r,64) && valid(r);
  }
  bool latest(Type type, JournalRecord& r, uint32_t skip=0) {
    if (!readable()) return false;
    for (uint32_t address=next_; address; ) {
      address-=64;
      if (at(address,r) && r.type==type) { if (!skip) return true; --skip; }
    }
    return false;
  }
  bool readable() const { return status_==Status::Ready || status_==Status::Full || status_==Status::IoError; }
  Status status() const { return status_; }
  uint32_t used() const { return next_; }
  uint32_t capacity() const { return flash_.size(); }
  uint32_t sessions() const { return count_; }
 private:
  struct Bytes { const uint8_t* data; const uint8_t* begin() const { return data; } const uint8_t* end() const { return data+64; } };
  static Bytes reinterpretBytes(const JournalRecord& r) { return {reinterpret_cast<const uint8_t*>(&r)}; }
  static constexpr uint32_t Magic=0x31544e5a, Commit=0x434f4d54; // ZNT1 / COMT
  JournalFlash& flash_;
  Status status_=Status::Unavailable;
  uint32_t next_=0, sequence_=0, count_=0;
};
