# Tatsächlicher Buildnachweis

Datum: 2026-10-11T20:56:19.872910+00:00

**Erfolgreich validiert und tatsächlich für ESP8266 kompiliert.**

| Einstellung | Wert |
|---|---|
| ESPHome | 2026.9.1 |
| Board / CPU / Flash | nodemcuv2 / 80 MHz / 4 MB |
| Arduino-ESP8266 | 3.1.2 |
| PlatformIO espressif8266 | 4.2.1 |
| GxEPD2 / Adafruit GFX / BusIO | 1.6.9 / 1.12.4 / 1.17.4 |
| API und OTA | verschlüsselt, gemeinsamer API-Schlüssel |

Lokaler Prüflauf: `python scripts/ci_build.py`, mit ESPHome aus der virtuellen
Umgebung. Die Komponente wird dabei aus dem lokalen Repository geladen.
Validierung und echter Cross-Build wurden erfolgreich beendet; keine
Compilerfehler. Die feste GitHub-Quelle steht in `bilderrahmen.yaml`.

| Messung | Byte | Grenze |
|---|---:|---:|
| Statisches RAM (DATA+RODATA+BSS, PlatformIO) | 31612 | 81920 |
| IRAM-Code (.text + .text1) | 27803 | 32768 |
| Flash (PlatformIO) | 387179 | 1044464 |
| BIN-Datei | 391328 | — |
| GxEPD2-Seitenpuffer im dynamischen Komponentenobjekt | 1600 | — |

.noinit zusätzlich: 56 Byte. Die IRAM-Abschnitte sind durch
2 Byte Alignment getrennt. Die Flashgrenze ist die OTA-taugliche
Anwendungsgrenze, nicht die gesamte 4-MB-Kapazität.

Der Laufzeit-Heap ist nicht aus der statischen RAM-Messung ableitbar:
Komponenten, WLAN und API benötigen dynamischen Speicher. Der Seitenpuffer
ist Teil des dynamisch angelegten Komponentenobjekts. Am Gerät den
Debug-Sensor vor/nach mehreren Aktualisierungen prüfen.

Die erzeugte BIN bleibt lokal als Build-Artefakt: sie enthält ausschließlich
synthetische Test-Secrets. Für Installation mit dem vorhandenen WLAN und
Home Assistant selbst mit den bestehenden Secrets kompilieren.

Es wurde kein Gerät geflasht. Sämtliche Hardwaretests sind offen, siehe
[HARDWARE_TESTS.md](HARDWARE_TESTS.md).

## Regression in v1.0.1

Am 12. Oktober 2026 wurde ein frischer ESP8266-Build ohne vorhandene
Compilerobjekte und ohne `platformio_options` in der Test-YAML erfolgreich
durchgeführt. Die Komponente setzt `lib_ldf_mode: chain+` selbst. Adafruit
GFX und BusIO wurden neu kompiliert; der Headerfehler tritt im Test nicht auf.
RAM: 31612 Byte; Flash: 387179 Byte. Hardwaretests bleiben offen.

## v1.0.2 — Diagnose und Home-Assistant-Test

ESPHome 2026.9.1: tatsächlich erfolgreich kompiliert. Statisches RAM:
31784 Byte; Flash: 389367 Byte. GxEPD2 bleibt bei 16 Zeilen / 1600 Byte.
Blueprint-Metadaten, Inputs, Templates und Automation-/Aktionsschema wurden
gegen eine isolierte Home-Assistant-2026.10.1-Installation validiert.
Keine Aktionen auf echten Geräten ausgeführt; der Live-Automationstest ist offen.
BUSY nach Hibernate ist jetzt nur eine Diagnose. Zeitmessungen sind pro
Bibliotheksaufruf; sie ersetzen keine eindeutige Timeout-Rückmeldung.
