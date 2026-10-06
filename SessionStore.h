#pragma once
#include <Arduino.h>
#include <Adafruit_SPIFlash.h>
#include "SessionJournal.h"
#include "TimerCore.h"

class QspiJournalFlash : public JournalFlash {
 public:
  QspiJournalFlash() : flash_(&transport_) {}
  bool begin() {
    // Core 1.1.13's bundled autodetection list omits P25Q16H.
    // Values: Adafruit_SPIFlash/src/flash_devices.h, P25Q16H descriptor.
    static const SPIFlash_Device_t device={1UL<<21,5000,0x85,0x60,0x15,32,0x02,
      false,true,true,true,false,false,false};
    delay(5);
    available_=flash_.begin(&device,1); return available_;
  }
  uint32_t size() const override { return available_ ? const_cast<Adafruit_SPIFlashBase&>(flash_).size() : 0; }
  bool read(uint32_t address, void* data, uint32_t length) override {
    return flash_.readBuffer(address,static_cast<uint8_t*>(data),length)==length;
  }
  bool write(uint32_t address, const void* data, uint32_t length) override {
    const bool ok=flash_.writeBuffer(address,static_cast<const uint8_t*>(data),length)==length;
    flash_.waitUntilReady(); return ok;
  }
 private:
  Adafruit_FlashTransport_QSPI transport_;
  Adafruit_SPIFlashBase flash_;
  bool available_=false;
};
struct DisplayPreferences { bool time=true, stones=true; uint8_t brightness=70; };
class SessionStore {
 public:
  explicit SessionStore(TimerCore& timer) : timer_(timer), journal_(flash_) {}
  void begin(uint32_t now);
  void tick(uint32_t now);
  void beforeAction(uint32_t now);
  void afterAction(uint32_t now);
  void saveSettings();
  bool command(const char* command, uint32_t now);
  void exportNext();
  void startExport();
  bool exporting() const { return exporting_; }
  DisplayPreferences preferences;
  SessionJournal& journal() { return journal_; }
  const char* statusText() const;
  bool dateKnown() const { return utcAnchor_!=0; }
 private:
  uint64_t utc() const { return utcAnchor_ ? utcAnchor_+(uptime_-anchorUptime_)/1000 : 0; }
  void checkpoint(uint8_t type, uint8_t outcome=0);
  void printRecord(const JournalRecord& r);
  void printInfo();
  TimerCore& timer_;
  QspiJournalFlash flash_;
  SessionJournal journal_;
  JournalRecord active_={};
  bool activeSession_=false, exporting_=false;
  uint32_t boot_=0, lastNow_=0, checkpointAt_=0, exportOffset_=0, exportLimit_=0;
  uint32_t priorElapsed_=0, priorDuration_=1200;
  TimerCore::State priorState_=TimerCore::State::Ready;
  uint64_t uptime_=0, utcAnchor_=0, anchorUptime_=0;
};
