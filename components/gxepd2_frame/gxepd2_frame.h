#pragma once
#include "esphome/core/component.h"
#include <GxEPD2_BW.h>
#include "esphome/components/http_request/http_request.h"
#include "image_protocol.h"

namespace esphome {
namespace gxepd2_frame {
class GxEPD2Frame : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }
  void request_test_image();
  void request_image_url(const std::string &url);
  void set_http_request(http_request::HttpRequestComponent *http) { http_ = http; }
  bool is_rendering() const { return rendering_; }

 protected:
  static void busy_callback_(const void *parameter);
  void draw_page_();
  void finish_();
  void begin_operation_();
  void log_operation_(const char *name);
  void image_loop_();
  bool read_image_bytes_(size_t goal);
  void abort_image_(const char *reason);
  void close_image_();
  // Allocated once as part of the component, never on the loop stack.
  GxEPD2_BW<GxEPD2_750_T7, 16> display_{GxEPD2_750_T7(15, 4, 5, 12)};
  enum class Phase : uint8_t { IDLE, START, PAGE, HIBERNATE, HTTP_OPEN, HTTP_HEADER, HTTP_INIT, HTTP_BODY, HTTP_REFRESH };
  Phase phase_{Phase::IDLE};
  bool rendering_{false};
  bool deadline_exceeded_{false};
  uint32_t started_at_{0};
  uint32_t deadline_ms_{60000};
  http_request::HttpRequestComponent *http_{nullptr};
  std::shared_ptr<http_request::HttpContainer> response_;
  std::string image_url_;
  // One bounded transfer strip in addition to the existing 1600-byte GxEPD2 buffer.
  uint8_t transfer_buffer_[image_protocol::STRIP_SIZE];
  size_t filled_{0};
  size_t payload_read_{0};
  uint32_t crc_{0xffffffffU};
  uint32_t expected_crc_{0};
  uint32_t last_data_ms_{0};
  bool image_failed_{false};
  uint32_t operation_started_at_{0};
  uint32_t busy_callback_us_{0};
  uint32_t busy_callback_count_{0};
};
}  // namespace gxepd2_frame
}  // namespace esphome
