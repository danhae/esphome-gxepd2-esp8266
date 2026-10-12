# ESPHome GxEPD2 — ESP8266 E-Paper-Bilderrahmen

NodeMCU AMICA / ESP8266, Waveshare 7,5 Zoll 800×480 Schwarz-Weiß,
Driver HAT Rev. 2.2. Kein ESP32-Support.

## E-Ink Dashboard verbinden

Die installierte HACS-Integration `cryptomilk/hass-eink-dashboard` übernimmt
die Bildgestaltung. [Verbindungsanleitung](docs/EINK_DASHBOARD.md): zwei HA-
Dateien kopieren, Image-Entität und Bildbutton im neuen Script auswählen.
Die Dashboard-ID wird automatisch ermittelt; das Script lädt und konvertiert
PNG und drückt erst danach den Bildbutton. Kein zusätzlicher Renderer.

## Eigene Bilder (v1.1.0-beta.2)

Fotos, Text und beliebige Grafiken außerhalb des ESP8266 als Bild vorbereiten.
Die Firmware lädt eine **EPD1-Datei** von einer frei einstellbaren lokalen
HTTP-Adresse und überträgt sie in 16-Zeilen-Streifen. Nach Länge/CRC32-Prüfung
folgen Full-Refresh und Hibernate. Ein neues Bild braucht keinen Firmwarebuild.

**Vorschauversion:** Der bekannte Testbildpfad wurde am Benutzergerät mehrfach
bestätigt. Die neue HTTP-Bildübertragung wurde in Simulation getestet, aber
noch nicht mit ESP8266-Toolchain kompiliert oder am realen Display ausgeführt.
Der Benutzer übernimmt den Firmwarebuild. CI validiert/generiert nur Quellcode
und prüft Konverter/Host-Simulation; es baut keine ESP8266-Firmware.

- [Vollständige Firmware-YAML](bilderrahmen.yaml)
- [Schritt-für-Schritt-Bildanleitung](docs/IMAGES.md)
- [Fertiges Demo-EPD1-Bild](images/demo.epd) und [Vorschau](images/demo.png)
- [PNG/JPEG-Konverter](tools/prepare_image.py)
- [Optionales HA-Package zur Bildaufbereitung](homeassistant/epaper_images_package.yaml)

In HA: **E-Paper Bild-URL** einmal einstellen, **E-Paper Bild aktualisieren**
drücken. Der **E-Paper Testbild anzeigen**-Button bleibt zur Diagnose erhalten.
Die drei Secrets `wifi_ssid`, `wifi_password`, `api_encryption_key` weiterverwenden.
OTA übernimmt den API-Schlüssel. Keine echten Zugangsdaten im Repository.

## Hardware unverändert

| HAT | NodeMCU | GPIO |
|---|---|---:|
| DIN | D7 | 13 |
| CLK | D5 | 14 |
| CS | D8 | 15 |
| DC | D2 | 4 |
| RST | D1 | 5 |
| BUSY | D6 | 12 |
| GND | GND | — |
| VCC | 3V3 | — |

nodemcuv2, 4 MB Flash, 80 MHz; Display Config B / 0.47R, 4-Line SPI.
GPIO15 muss beim Reset LOW bleiben. Kein zusätzlicher ESPHome-SPI-Block.
GxEPD2 verwaltet Hardware-SPI, Reset 2 ms, Rotation 0.

## Speicher und feste Abhängigkeiten

Exakt GxEPD2_BW<GxEPD2_750_T7, 16>: 1600 Byte GxEPD2-Puffer.
Zusätzlich 1600 Byte HTTP-Empfangspuffer, insgesamt 3200 Byte für Pixel-/
Transferpuffer. TCP/HTTP benötigt weiteren begrenzten Verwaltungs-/Heap-Speicher.
Kein 48000-Byte-Framebuffer und keine Bilddatei im Flash.

ESPHome 2026.9.1; empfohlenes Arduino-ESP8266 3.1.2;
GxEPD2 1.6.9; Adafruit GFX 1.12.4; BusIO 1.17.4. Wire ist eine Header-Abhängigkeit;
I2C wird nicht initialisiert. Die Komponente aktiviert PlatformIO chain+ selbst.
Die frühere Angabe GxEPD2 3.1.3 lässt sich in den offiziellen Releases nicht bestätigen.

## Tests und Einschränkungen

[Build-/Validierungsnachweise](docs/BUILD.md), [Hardwarecheckliste](docs/HARDWARE_TESTS.md).
Historische v1.0.x-Builds sind echte Firmwarebuilds; diese neue Version wurde
auf Wunsch des Benutzers nicht als Firmware kompiliert.

```sh
python -m pip install -r requirements.txt
python scripts/ci_build.py  # nur Validierung und Quellgenerierung
python -m unittest discover -s tests -v
python scripts/test_stream.py  # hostseitige C++-Simulation, keine ESP-Firmware
```

Das Display hat weiterhin mehrere Sekunden physische Refreshzeit. HTTP-Open
und Refresh können die API vorübergehend verzögern. BUSY nach Hibernate wird
nur protokolliert; `Hibernate gesendet` ist kein gemessener Schlafnachweis.
Für Diagnose `logger.level: DEBUG`. Kein Webserver auf dem ESP8266, kein
MQTT, Captive Portal, Deep Sleep oder Decoder auf dem Gerät.

## Herkunft und Lizenz

Eigenständige ESP8266-Implementierung zur Aufgabenstellung des
[ESP32-Originalprojekts](https://github.com/Smengerl/ePaper_frame); keine
Implementierungsdateien daraus übernommen. GxEPD2 wird als Bibliothek geladen.
[ESPHome](https://github.com/esphome/esphome/tree/2026.9.1),
[GxEPD2](https://github.com/ZinggJM/GxEPD2/tree/1.6.9).
Projekt GPL-3.0, siehe [LICENSE](LICENSE); Abhängigkeiten behalten ihre Lizenzen.
