#pragma once
#include <stdint.h>
#include <stdio.h>
#include <sstream>
struct TestSerial {
  std::ostringstream output;
  explicit operator bool() const { return true; }
  template<class T> void print(const T& value) { output<<value; }
  void print(uint8_t value) { output<<unsigned(value); }
  template<class T> void println(const T& value) { print(value); output<<'\n'; }
};
extern TestSerial Serial;
inline void delay(unsigned) {}
struct TestFicr { uint32_t DEVICEID[2]={123,456}; };
extern TestFicr* NRF_FICR;
