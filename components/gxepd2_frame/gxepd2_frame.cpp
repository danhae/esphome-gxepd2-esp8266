#include "gxepd2_frame.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include <SPI.h>

namespace esphome {
namespace gxepd2_frame {
static const char *const TAG = "gxepd2_frame";

void GxEPD2Frame::setup() {
  // GxEPD2 alone owns Arduino hardware SPI: CLK=14, MOSI=13, MISO=12.
  // BUSY shares the default MISO pin, but this display never reads SPI data.
  SPI.begin();
  display_.epd2.setBusyCallback(busy_callback_, this);
  // 0 suppresses GxEPD2's Serial.begin; ESPHome owns the serial logger.
  // Keep the proven initial=true, reset=2 ms, pulldown=false settings.
  display_.init(0, true, 2, false);
  display_.setRotation(0);
  ESP_LOGD(TAG, "Initialisiert; Heap=%u, groesster Block=%u",
           ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
}

void GxEPD2Frame::dump_config() {
  ESP_LOGCONFIG(TAG, "GxEPD2_750_T7: 800x480, 16 Zeilen / 1600 Byte Bildpuffer");
  ESP_LOGCONFIG(TAG, "SPI: CLK=14 MOSI=13; CS=15 DC=4 RST=5 BUSY=12 (aktiv LOW)");
}

void GxEPD2Frame::request_test_image() {
  if (rendering_) {
    ESP_LOGW(TAG, "Testbild bereits aktiv; weiterer Aufruf verworfen");
    return;
  }
  if (ESP.getFreeHeap() < 8192 || ESP.getMaxFreeBlockSize() < 4096) {
    ESP_LOGW(TAG, "Zu wenig Heap: frei=%u, Block=%u; Auftrag verworfen",
             ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
    return;
  }
  rendering_ = true;  // Set in the button action, before returning to the API.
  busy_failed_ = false;
  started_at_ = millis();
  phase_ = Phase::START;
  ESP_LOGW(TAG, "Testbild startet; Heap=%u", ESP.getFreeHeap());
}

void GxEPD2Frame::busy_callback_(const void *parameter) {
  auto *self = const_cast<GxEPD2Frame *>(static_cast<const GxEPD2Frame *>(parameter));
  // The driver has a 10-second BUSY timeout. A stuck LOW after a call is
  // checked in loop(). Never recursively call App.loop() from this callback.
  App.feed_wdt();
  delay(1);  // ESP8266 SDK/Wi-Fi and watchdog get CPU time.
  if (self->rendering_ && millis() - self->started_at_ > 60000)
    self->busy_failed_ = true;
}

void GxEPD2Frame::draw_page_() {
  display_.fillScreen(GxEPD_WHITE);
  display_.drawRect(10, 10, display_.width() - 20, display_.height() - 20, GxEPD_BLACK);
  display_.fillRect(40, 40, 200, 100, GxEPD_BLACK);
  for (int y = 180; y < 420; y += 40) {
    for (int x = 40; x < 760; x += 40) {
      if (((x / 40) + (y / 40)) % 2 == 0)
        display_.fillRect(x, y, 40, 40, GxEPD_BLACK);
    }
  }
  display_.setTextColor(GxEPD_BLACK);
  display_.setTextSize(4);
  display_.setCursor(280, 100);
  display_.print(F("EPAPER TEST"));
}

void GxEPD2Frame::finish_() {
  ESP_LOGW(TAG, "%s; Dauer=%u ms, Heap=%u",
           busy_failed_ ? "Displayfehler/BUSY; Auftrag beendet" : "Testbild beendet, Hibernate",
           static_cast<unsigned>(millis() - started_at_), ESP.getFreeHeap());
  rendering_ = false;
  phase_ = Phase::IDLE;
}

void GxEPD2Frame::loop() {
  if (!rendering_) return;
  App.feed_wdt();
  if (millis() - started_at_ > 60000) {
    busy_failed_ = true;
    phase_ = Phase::HIBERNATE;
  }
  switch (phase_) {
    case Phase::START:
      // Reinitialization is safe after hibernate and after a failed attempt.
      display_.init(0, true, 2, false);
      display_.setFullWindow();
      display_.firstPage();
      phase_ = Phase::PAGE;
      break;
    case Phase::PAGE: {
      draw_page_();
      const bool more = display_.nextPage();
      // T7 BUSY is active LOW. GxEPD2 returns after its bounded timeout,
      // but has no success return value for the physical refresh.
      if (digitalRead(12) == LOW) busy_failed_ = true;
      if (!more || busy_failed_) phase_ = Phase::HIBERNATE;
      break;
    }
    case Phase::HIBERNATE:
      display_.hibernate();
      if (digitalRead(12) == LOW) busy_failed_ = true;
      finish_();
      break;
    case Phase::IDLE:
      break;
  }
  App.feed_wdt();
  yield();
}
}  // namespace gxepd2_frame
}  // namespace esphome
