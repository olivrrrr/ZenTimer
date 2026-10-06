#include "../SessionJournal.h"
#include <assert.h>
#include <vector>
#include <algorithm>
struct Flash : JournalFlash {
  std::vector<uint8_t> bytes;
  int writeBudget=-1;
  Flash(unsigned slots) : bytes(slots*64,255) {}
  uint32_t size() const override { return bytes.size(); }
  bool read(uint32_t a,void* p,uint32_t n) override { if(a+n>size())return false; memcpy(p,bytes.data()+a,n); return true; }
  bool write(uint32_t a,const void* p,uint32_t n) override {
    if(a+n>size())return false;
    const auto* source=static_cast<const uint8_t*>(p);
    for(unsigned i=0;i<n;++i) {
      if(writeBudget==0)return false;
      assert((bytes[a+i]&source[i])==source[i]);
      bytes[a+i]&=source[i]; if(writeBudget>0)--writeBudget;
    }
    return true;
  }
};
int main() {
  Flash original(8); SessionJournal j(original); assert(j.mount());
  JournalRecord r={}; r.type=SessionJournal::End; r.session=42; r.elapsedMs=12000; assert(j.append(r));
  auto stable=original.bytes;
  // Every interruption point in a 64-byte append must preserve the old session.
  for(int cut=0;cut<64;++cut) {
    Flash f=original; f.writeBudget=cut; SessionJournal writing(f); assert(writing.mount());
    JournalRecord added={}; added.type=SessionJournal::End; added.session=43;
    assert(!writing.append(added)); f.writeBudget=-1;
    SessionJournal reboot(f);
    assert(reboot.mount()); JournalRecord previous;
    assert(reboot.sessions()==1); assert(reboot.latest(SessionJournal::End,previous)); assert(previous.session==42);
    JournalRecord next={}; next.type=SessionJournal::End; next.session=44; assert(reboot.append(next));
    assert(std::equal(stable.begin(),stable.begin()+128,f.bytes.begin()));
  }
  Flash foreign(8); foreign.bytes[256]=0; auto copy=foreign.bytes;
  SessionJournal blocked(foreign); assert(!blocked.mount()); assert(blocked.status()==SessionJournal::Status::Foreign);
  assert(foreign.bytes==copy);
  Flash small(2); SessionJournal full(small); assert(full.mount()); r={}; r.type=SessionJournal::End; assert(full.append(r));
  assert(full.status()==SessionJournal::Status::Full); assert(!full.append(r));
  SessionJournal after(small); assert(after.mount()); assert(after.sessions()==1); assert(after.latest(SessionJournal::End,r));
  Flash corrupt=original; corrupt.bytes[64+24]^=1; SessionJournal recovery(corrupt); assert(recovery.mount()); assert(recovery.sessions()==0);
}
