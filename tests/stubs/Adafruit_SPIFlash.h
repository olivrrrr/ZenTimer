#pragma once
#include <stdint.h>
#include <string.h>
#include <vector>
struct SPIFlash_Device_t { uint32_t size; uint16_t startup; uint8_t manufacturer,type,capacity,clock,quad; bool a,b,c,d,e,f,g; };
class Adafruit_FlashTransport_QSPI {};
extern std::vector<uint8_t> testFlash;
class Adafruit_SPIFlashBase {
 public:
  explicit Adafruit_SPIFlashBase(Adafruit_FlashTransport_QSPI*) {}
  bool begin(const SPIFlash_Device_t*,unsigned) { return true; }
  uint32_t size() const { return testFlash.size(); }
  void waitUntilReady() {}
  uint32_t readBuffer(uint32_t a,uint8_t* p,uint32_t n) { if(a+n>size())return 0; memcpy(p,testFlash.data()+a,n); return n; }
  uint32_t writeBuffer(uint32_t a,const uint8_t* p,uint32_t n) { if(a+n>size())return 0; for(unsigned i=0;i<n;++i)testFlash[a+i]&=p[i]; return n; }
};
