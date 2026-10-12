#pragma once
#include "Arduino.h"
#include <vector>
#include <cassert>
constexpr int GxEPD_WHITE=1, GxEPD_BLACK=0;
inline unsigned fake_refreshes=0, fake_hibernates=0, fake_strips=0;
inline std::vector<uint8_t> fake_controller;
class GxEPD2_750_T7 { public:
 GxEPD2_750_T7(int,int,int,int) {}
 void setBusyCallback(void (*)(const void*),const void*) {}
 void writeImage(const uint8_t *data,int x,int y,int w,int h,bool,bool,bool) {
  assert(x==0 && w==800 && h==16 && y==int(fake_strips*16));
  fake_strips++; fake_controller.insert(fake_controller.end(),data,data+1600); fake_now+=10;
 }
 void refresh(bool partial) { assert(!partial); fake_refreshes++; fake_now+=6700; }
};
template<class T,unsigned H> class GxEPD2_BW { public:
 T epd2;
 explicit GxEPD2_BW(T epd):epd2(epd) {}
 void init(unsigned,bool,unsigned,bool) {}
 void setRotation(int) {} void setFullWindow() {} void firstPage() {}
 bool nextPage() { return false; }
 void fillScreen(int) {} int width() { return 800; } int height() { return 480; }
 void drawRect(int,int,int,int,int) {} void fillRect(int,int,int,int,int) {}
 void setTextColor(int) {} void setTextSize(int) {} void setCursor(int,int) {} void print(const char*) {}
 void hibernate() { fake_hibernates++; }
};
