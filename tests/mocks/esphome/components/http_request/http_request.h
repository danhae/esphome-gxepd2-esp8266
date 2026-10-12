#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <algorithm>
#include <vector>
namespace esphome::http_request {
class HttpContainer { public:
 virtual ~HttpContainer()=default;
 size_t content_length=48016;
 int status_code=200;
 virtual int read(uint8_t*,size_t)=0;
 virtual void end()=0;
 virtual bool is_read_complete() const=0;
};
class TestResponse : public HttpContainer { public:
 std::vector<uint8_t> body; size_t pos=0; unsigned calls=0; bool stalled=false, ended=false;
 explicit TestResponse(std::vector<uint8_t> bytes):body(std::move(bytes)) {}
 int read(uint8_t *out,size_t max) override {
  calls++;
  if (stalled || calls%3==0) return 0;
  if (pos==body.size()) return -1;
  const size_t n=std::min({max,size_t(37),body.size()-pos});
  std::copy_n(body.data()+pos,n,out); pos+=n; return int(n);
 }
 void end() override { ended=true; }
 bool is_read_complete() const override { return !stalled && pos==body.size(); }
};
class HttpRequestComponent { public:
 std::shared_ptr<TestResponse> response;
 unsigned requests=0;
 std::shared_ptr<HttpContainer> get(const std::string&) { requests++; return response; }
};
}
