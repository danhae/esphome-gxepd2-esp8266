# Changelog

## v1.1.0-beta.1

- Local HTTP EPD1 image source, URL text entity and update-image button.
- Fixed-size strip transfer to controller, CRC32 and length validation before refresh.
- PNG/JPEG/Pillow converter, demo image and optional Home Assistant preparation script.
- Host streaming/error tests and conversion tests; schema/source generation only.
- No ESP8266 firmware build for this preview version; user compiles and tests hardware.

## v1.0.2

- BUSY after nextPage/power-off/hibernate is no longer treated as a definitive display error.
- Per-call DEBUG timings and BUSY callback counts; slow calls warn about possible timeout only.
- Rendering binary sensor and manual Home Assistant three-refresh test blueprint.
- Hibernate completion wording reports the command, not physical confirmation.

## v1.0.1

- The component enables PlatformIO chain+ dependency discovery itself.
- Regression build omits the YAML platformio_options block to verify BusIO header resolution.
- Existing v1.0.0 tag remains unchanged.

## v1.0.0

- ESP8266-only external component for ESPHome 2026.9.1.
- Confirmed display type GxEPD2_BW<GxEPD2_750_T7, 16>, 1600-byte page buffer.
- Fixed NodeMCU pin mapping and 2 ms reset pulse.
- Home Assistant test-image button and free-heap debug sensor.
- Encrypted native API and encrypted ESPHome OTA using the same secret key.
- Paged rendering, repeat-call guard, watchdog feeding and bounded BUSY waits.
- Pinned Arduino library versions and PlatformIO chain+ dependency discovery.
- Build validation; hardware acceptance remains pending.
