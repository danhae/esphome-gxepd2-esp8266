#pragma once
#include "esphome/core/component.h"
#include <GxEPD2_BW.h>

namespace esphome {
namespace gxepd2_frame {
class GxEPD2Frame : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }
  void request_test_image();
  bool is_rendering() const { return rendering_; }

 protected:
  static void busy_callback_(const void *parameter);
  void draw_page_();
  void finish_();
  void begin_operation_();
  void log_operation_(const char *name);
  // Allocated once as part of the component, never on the loop stack.
  GxEPD2_BW<GxEPD2_750_T7, 16> display_{GxEPD2_750_T7(15, 4, 5, 12)};
  enum class Phase : uint8_t { IDLE, START, PAGE, HIBERNATE };
  Phase phase_{Phase::IDLE};
  bool rendering_{false};
  bool deadline_exceeded_{false};
  uint32_t started_at_{0};
  uint32_t operation_started_at_{0};
  uint32_t busy_callback_us_{0};
  uint32_t busy_callback_count_{0};
};
}  // namespace gxepd2_frame
}  // namespace esphome
