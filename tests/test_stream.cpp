#include "gxepd2_frame.h"
#include <cassert>
#include <iostream>
using namespace esphome::gxepd2_frame;
using namespace esphome::http_request;
std::vector<uint8_t> image() {
 std::vector<uint8_t> out(48016);
 out[0]='E';out[1]='P';out[2]='D';out[3]='1';out[4]=0x20;out[5]=3;out[6]=0xe0;out[7]=1;
 out[8]=0x80;out[9]=0xbb;
 for (size_t i=16;i<out.size();i++) out[i]=uint8_t(i*17);
 auto crc=image_protocol::crc_update(0xffffffffU,out.data()+16,48000)^0xffffffffU;
 for (int i=0;i<4;i++) out[12+i]=uint8_t(crc>>(8*i));
 return out;
}
void reset() { fake_now=0;fake_refreshes=0;fake_strips=0;fake_hibernates=0;fake_controller.clear();fake_free_heap=42000; }
void drain(GxEPD2Frame &frame,unsigned step=1) {
 unsigned loops=0;
 while(frame.is_rendering() && loops++<20000) { frame.loop(); fake_now+=step; }
 assert(!frame.is_rendering());
}
void transfer(std::vector<uint8_t> data,bool success,int status=200,bool stalled=false,unsigned step=1) {
 reset();
 HttpRequestComponent http;http.response=std::make_shared<TestResponse>(data);http.response->status_code=status;http.response->stalled=stalled;
 GxEPD2Frame frame;frame.set_http_request(&http);frame.setup();
 frame.request_image_url("http://homeassistant.local:8123/local/frame.epd");
 assert(frame.is_rendering());
 frame.request_test_image();frame.request_image_url("http://different.local/ignored.epd");
 drain(frame,step);
 assert(http.requests==1 && http.response->ended && fake_hibernates==1);
 assert(fake_refreshes==(success?1U:0U));
 if(success) { assert(fake_strips==30);assert(fake_controller==std::vector<uint8_t>(data.begin()+16,data.end())); }
}
int main() {
 const uint8_t known[]={'1','2','3','4','5','6','7','8','9'};
 assert((image_protocol::crc_update(0xffffffffU,known,9)^0xffffffffU)==0xcbf43926U);
 auto valid=image();transfer(valid,true);
 auto bad=valid;bad.back()^=1;transfer(bad,false);
 bad=valid;bad.resize(2000);transfer(bad,false);
 bad=valid;bad[0]='X';transfer(bad,false);
 transfer(valid,false,404);
 transfer(valid,false,200,true,100);
 transfer(valid,false,200,false,100); // total 120s deadline despite small periodic reads
 reset();HttpRequestComponent http;http.response=std::make_shared<TestResponse>(valid);
 GxEPD2Frame frame;frame.set_http_request(&http);frame.setup();
 frame.request_image_url("https://unsupported.local/frame.epd");assert(!frame.is_rendering() && http.requests==0);
 frame.request_image_url("http://user:password@host/frame.epd");assert(!frame.is_rendering());
 fake_free_heap=8000;frame.request_image_url("http://host/frame.epd");assert(!frame.is_rendering());
 fake_free_heap=42000;frame.request_test_image();drain(frame);assert(fake_hibernates==1); // BUSY stays LOW in this simulation
 std::cout<<"Streaming tests: CRC, fragmented reads, truncation, stalls, deadlines, guard and LOW after hibernate passed.\n";
}
