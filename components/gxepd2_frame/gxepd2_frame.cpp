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
  deadline_exceeded_ = false;
  image_failed_ = false;
  deadline_ms_ = 60000;
  started_at_ = millis();
  phase_ = Phase::START;
  ESP_LOGW(TAG, "Testbild startet; Heap=%u", ESP.getFreeHeap());
}

void GxEPD2Frame::request_image_url(const std::string &url) {
  if (rendering_) {
    ESP_LOGW(TAG, "Displayauftrag bereits aktiv; Bildaufruf verworfen");
    return;
  }
  if (http_ == nullptr || url.size() > 255 || url.compare(0, 7, "http://") != 0 ||
      url.find('@') != std::string::npos || url.find_first_of(" \r\n\t") != std::string::npos) {
    ESP_LOGE(TAG, "Bild-URL: lokale http://-Adresse ohne Zugangsdaten erforderlich (max. 255 Zeichen)");
    return;
  }
  if (ESP.getFreeHeap() < 16000 || ESP.getMaxFreeBlockSize() < 8192) {
    ESP_LOGE(TAG, "Zu wenig Heap fuer HTTP; Bildaufruf verworfen");
    return;
  }
  image_url_ = url;
  rendering_ = true;
  image_failed_ = false;
  deadline_exceeded_ = false;
  deadline_ms_ = 120000;
  started_at_ = millis();
  filled_ = 0;
  payload_read_ = 0;
  crc_ = 0xffffffffU;
  phase_ = Phase::HTTP_OPEN;
  ESP_LOGW(TAG, "Eigenes Bild startet; Heap=%u", ESP.getFreeHeap());
}

void GxEPD2Frame::close_image_() {
  if (response_) {
    response_->end();
    response_.reset();
  }
  image_url_.clear();
}

void GxEPD2Frame::abort_image_(const char *reason) {
  ESP_LOGE(TAG, "Bildtransfer abgebrochen: %s; kein Bild-Refresh ausgeloest", reason);
  image_failed_ = true;
  close_image_();
  phase_ = Phase::HIBERNATE;
}

bool GxEPD2Frame::read_image_bytes_(size_t goal) {
  const int n = response_->read(transfer_buffer_ + filled_, goal - filled_);
  if (n > 0) {
    filled_ += static_cast<size_t>(n);
    last_data_ms_ = millis();
  } else if (n < 0 || response_->is_read_complete()) {
    abort_image_("Verbindung beendet, Datei unvollstaendig");
    return false;
  } else if (millis() - last_data_ms_ >= 10000) {
    abort_image_("10 Sekunden ohne Bilddaten");
    return false;
  }
  return filled_ == goal;
}

void GxEPD2Frame::image_loop_() {
  switch (phase_) {
    case Phase::HTTP_OPEN:
      // No body capture or full-image allocation. HTTP connection/header open is synchronous.
      response_ = http_->get(image_url_);
      if (!response_ || response_->status_code != 200 ||
          response_->content_length != image_protocol::HEADER_SIZE + image_protocol::PAYLOAD_SIZE) {
        abort_image_("HTTP 200 und Content-Length 48016 erforderlich");
        return;
      }
      last_data_ms_ = millis();
      phase_ = Phase::HTTP_HEADER;
      return;
    case Phase::HTTP_HEADER:
      if (!read_image_bytes_(image_protocol::HEADER_SIZE)) return;
      if (!image_protocol::valid_header(transfer_buffer_)) {
        abort_image_("Kein gueltiges EPD1-Bild mit 800x480 Pixeln");
        return;
      }
      expected_crc_ = image_protocol::read32(transfer_buffer_ + 12);
      filled_ = 0;
      phase_ = Phase::HTTP_INIT;
      return;
    case Phase::HTTP_INIT:
      display_.init(0, true, 2, false);
      phase_ = Phase::HTTP_BODY;
      return;
    case Phase::HTTP_BODY:
      if (!read_image_bytes_(image_protocol::STRIP_SIZE)) return;
      crc_ = image_protocol::crc_update(crc_, transfer_buffer_, image_protocol::STRIP_SIZE);
      // Controller RAM receives 16 rows; the visible panel is refreshed only after CRC validation.
      begin_operation_();
      display_.epd2.writeImage(transfer_buffer_, 0, payload_read_ / 100, 800, 16, false, false, false);
      log_operation_("Bildstreifen");
      payload_read_ += image_protocol::STRIP_SIZE;
      filled_ = 0;
      if (payload_read_ == image_protocol::PAYLOAD_SIZE) {
        if ((crc_ ^ 0xffffffffU) != expected_crc_) {
          abort_image_("CRC32 stimmt nicht");
          return;
        }
        close_image_();  // Release TCP memory before the physical refresh.
        phase_ = Phase::HTTP_REFRESH;
      }
      return;
    case Phase::HTTP_REFRESH:
      ESP_LOGW(TAG, "48000 Bildbytes und CRC32 geprueft; Display-Refresh");
      begin_operation_();
      display_.epd2.refresh(false);
      log_operation_("Bildrefresh");
      phase_ = Phase::HIBERNATE;
      return;
    default:
      return;
  }
}

void GxEPD2Frame::busy_callback_(const void *parameter) {
  auto *self = const_cast<GxEPD2Frame *>(static_cast<const GxEPD2Frame *>(parameter));
  const uint32_t start = micros();
  App.feed_wdt();
  delay(1);  // SDK/Wi-Fi; never call App.loop() recursively.
  self->busy_callback_us_ += micros() - start;
  self->busy_callback_count_++;
  if (self->rendering_ && millis() - self->started_at_ > self->deadline_ms_)
    self->deadline_exceeded_ = true;
}

void GxEPD2Frame::begin_operation_() {
  operation_started_at_ = millis();
  busy_callback_us_ = 0;
  busy_callback_count_ = 0;
}

void GxEPD2Frame::log_operation_(const char *name) {
  const uint32_t duration = millis() - operation_started_at_;
  ESP_LOGD(TAG, "%s: Dauer=%u ms, BUSY-Callbacks=%u, Callbackzeit=%u ms, BUSY=%s",
           name, static_cast<unsigned>(duration), static_cast<unsigned>(busy_callback_count_),
           static_cast<unsigned>(busy_callback_us_ / 1000), digitalRead(12) == LOW ? "LOW" : "HIGH");
  if (duration >= 10000)
    ESP_LOGW(TAG, "%s dauerte >=10s; moeglicher BUSY-Timeout. GxEPD2 meldet echte Timeouts nur seriell.", name);
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
           image_failed_ ? "Bildtransfer abgebrochen; Hibernate gesendet" :
           deadline_exceeded_ ? "Auftragszeitlimit ueberschritten; Hibernate gesendet" : "Renderdurchlauf beendet; Hibernate gesendet",
           static_cast<unsigned>(millis() - started_at_), ESP.getFreeHeap());
  close_image_();
  rendering_ = false;
  phase_ = Phase::IDLE;
}

void GxEPD2Frame::loop() {
  if (!rendering_) return;
  App.feed_wdt();
  if (millis() - started_at_ > deadline_ms_) {
    deadline_exceeded_ = true;
    close_image_();
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
      begin_operation_();
      const bool more = display_.nextPage();
      log_operation_("nextPage");
      if (!more || deadline_exceeded_) phase_ = Phase::HIBERNATE;
      break;
    }
    case Phase::HIBERNATE:
      begin_operation_();
      display_.hibernate();
      log_operation_("hibernate");
      // BUSY after power-off/deep-sleep is diagnostic only, not a refresh result.
      finish_();
      break;
    case Phase::HTTP_OPEN:
    case Phase::HTTP_HEADER:
    case Phase::HTTP_INIT:
    case Phase::HTTP_BODY:
    case Phase::HTTP_REFRESH:
      image_loop_();
      break;
    case Phase::IDLE:
      break;
  }
  App.feed_wdt();
  yield();
}
}  // namespace gxepd2_frame
}  // namespace esphome
