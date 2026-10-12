# ESPHome + GxEPD2 auf ESP8266

Minimaler E-Paper-Test für **NodeMCU AMICA / ESP8266**, Waveshare **7,5 Zoll,
800 × 480, Schwarz-Weiß**, Driver HAT Rev. 2.2. Ein Button in Home Assistant
zeichnet ein Testbild; danach geht das Display in Hibernate. WLAN, API und
OTA bleiben Teil der Firmware.

**Software-Build geprüft. Hardwaretests stehen noch aus.** Die Displayklasse
und Verkabelung wurden im zugrunde liegenden Arduino-Test am Gerät bestätigt;
diese ESPHome-Implementierung wurde noch nicht am Gerät getestet.

## Schnellstart

1. ESPHome **2026.9.1** verwenden.
2. [bilderrahmen.yaml](bilderrahmen.yaml) vollständig nach
   `/config/esphome/bilderrahmen.yaml` kopieren.
3. In der vorhandenen `/config/esphome/secrets.yaml` müssen `wifi_ssid`,
   `wifi_password` und `api_encryption_key` existieren. Für den API-Key den
   **vorhandenen** Schlüssel des Geräts weiterverwenden. Falls er bisher
   direkt in der alten YAML steht, ihn als `api_encryption_key` in Secrets
   übertragen. Keine vorhandene Secrets-Datei überschreiben.
4. Validieren, installieren und das Gerät in Home Assistant öffnen.
5. **E-Paper Testbild anzeigen** drücken. **E-Paper Freier Heap** beobachten.

Die Komponente wird automatisch von GitHub geladen. Zusätzliche lokale
Komponentendateien sind für diesen Installationsweg nicht erforderlich.
Die YAML verwendet den festen Tag `v1.0.2`; Änderungen auf `main` werden
nicht ungefragt übernommen.

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/danhae/esphome-gxepd2-esp8266
      ref: v1.0.2
      path: components
    components: [gxepd2_frame]
```

Die vollständige Konfiguration steht in [bilderrahmen.yaml](bilderrahmen.yaml).
`ota.encryption` übernimmt in ESPHome 2026.9.1 den API-Schlüssel. Ein separates
OTA-Passwort ist in dieser Konfiguration nicht erforderlich. Das entspricht
der verschlüsselten OTA-Konfiguration des vorgesehenen Geräts.

## Hardware

| Waveshare HAT | NodeMCU | GPIO |
|---|---|---:|
| DIN | D7 | 13 |
| CLK | D5 | 14 |
| CS | D8 | 15 |
| DC | D2 | 4 |
| RST | D1 | 5 |
| BUSY | D6 | 12 |
| GND | GND | — |
| VCC | 3V3 | — |

Board `nodemcuv2`, 4 MB Flash, 80 MHz. HAT: Display Config **B / 0.47R**,
Interface **0 / 4-Line SPI**. GPIO15 ist ein Boot-Strapping-Pin und muss beim
Reset LOW bleiben. Die bereits getestete Verkabelung wird nicht geändert.

Keinen ESPHome-`spi:`-Block hinzufügen: GxEPD2 besitzt die Arduino-Hardware-SPI.
GPIO12 ist der Standard-MISO-Pin, wird hier aber ausschließlich als BUSY
verwendet; der Displaytreiber liest keine SPI-Daten. Reset 2 ms,
`pulldown_rst_mode=false`, Rotation 0.

## Speicher und Bibliotheken

Exakt `GxEPD2_BW<GxEPD2_750_T7, 16>`: **1600 Byte** Seitenpuffer, kein
48000-Byte-Fullscreen-Framebuffer. Das Komponentenobjekt wird einmal angelegt;
der Seitenpuffer liegt nicht auf dem loop()-Stack.

| Abhängigkeit | Version |
|---|---|
| ESPHome | 2026.9.1 |
| Arduino-ESP8266 | 3.1.2, empfohlen von ESPHome 2026.9.1 |
| GxEPD2 | 1.6.9 |
| Adafruit GFX | 1.12.4 |
| Adafruit BusIO | 1.17.4 |

Die Angabe „GxEPD2 3.1.3“ aus dem vorherigen Arduino-Test lässt sich in den
offiziellen GxEPD2-Releases nicht bestätigen. Dieses Projekt verwendet die
verfügbare feste Version 1.6.9 mit unveränderter Displayklasse.

Wire wird wegen der Adafruit-Header explizit eingebunden, I2C aber nicht
initialisiert. Die Komponente aktiviert `lib_ldf_mode: chain+` selbst; die YAML enthält
diese Einstellung zusätzlich ausdrücklich. Damit werden die Abhängigkeiten der Arduino-
Bibliotheken auf. Kein Fork und keine Änderungen an GxEPD2.

## Ablauf und Einschränkungen

Der Button setzt sofort eine Rendering-Sperre. Initialisierung, jeweils eine
Seite mit `firstPage()`/`nextPage()` und Hibernate laufen in unterschiedlichen
ESPHome-loop()-Durchläufen. Weitere verarbeitete Aufrufe während Rendering
werden verworfen. Bei weniger als 8192 Byte freiem Heap oder einem größten
freien Block unter 4096 Byte wird ein neuer Auftrag abgelehnt.

Der eigentliche Refresh bleibt **synchron**. Im BUSY-Callback werden der
Watchdog und das ESP8266-SDK bedient; die native ESPHome-API und OTA können
während eines solchen Aufrufs kurz verzögert reagieren. Keine rekursiven
Aufrufe von `App.loop()`.

GxEPD2 begrenzt eine einzelne BUSY-Wartezeit auf 10 Sekunden. BUSY-Pegel
nach einer Seite oder Hibernate werden nur diagnostisch protokolliert, nicht
pauschal als Displayfehler gewertet. DEBUG zeigt je nextPage-/Hibernate-Aufruf
die Dauer, Zahl der BUSY-Callbacks, Zeit innerhalb dieser Callbacks und den
abschließenden Pinpegel. Callbackzeit ist weder die komplette BUSY-Wartezeit
noch eine einzelne Wartephase: ein GxEPD2-Aufruf kann mehrere Wartephasen enthalten.
Aufrufe ab 10 Sekunden werden als langsame Aufrufe mit möglichem Timeout
gekennzeichnet. GxEPD2 stellt tatsächliche BUSY-Timeouts nicht als Rückgabewert
bereit; seine eindeutige Meldung `Busy Timeout!` ist über UART sichtbar.
Eine Auftragsdauer über 60 Sekunden wird zwischen Aufrufen erkannt; danach
wird Hibernate versucht. `Hibernate gesendet` bestätigt den Softwareaufruf,
nicht den physisch gemessenen Schlafzustand.

Standard-Logging: WARN, mit Start-, Ende- und Fehlerausgaben. Für Diagnose
`logger.level: DEBUG` setzen. Der Debug-Sensor liefert freien Heap alle
30 Sekunden; Home Assistant vergibt die tatsächlichen Entity-IDs.

## Build und Hardware-Abnahme

[Buildnachweis](docs/BUILD.md) und [Hardware-Checkliste](docs/HARDWARE_TESTS.md).
Die GitHub-Actions-Prüfung validiert und kompiliert mit synthetischen,
bei jedem Lauf neu erzeugten Test-Secrets. Sie flasht kein Gerät und
veröffentlicht keine BIN mit Zugangsdaten.

Lokaler Prüflauf:

```sh
python -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/python scripts/ci_build.py
```

Für den praktischen Einsatz stets mit den eigenen vorhandenen Secrets bauen.
Kein Webserver, Captive Portal, MQTT, Deep Sleep, Bilddownload, Decoder oder
ESP32-Support enthalten.

## Struktur

```text
bilderrahmen.yaml
components/gxepd2_frame/__init__.py
components/gxepd2_frame/gxepd2_frame.h
components/gxepd2_frame/gxepd2_frame.cpp
docs/BUILD.md
docs/HARDWARE_TESTS.md
scripts/ci_build.py
.github/workflows/build.yml
requirements.txt
CHANGELOG.md
LICENSE
```

## Quellen und Lizenz

Eigenständige ESP8266-Implementierung, inspiriert durch die Aufgabenstellung
zum [ESP32-Originalprojekt](https://github.com/Smengerl/ePaper_frame).
Keine Dateien aus dessen Implementierung übernommen.

- [ESPHome External Components](https://esphome.io/components/external_components/)
- [ESPHome Debug-Sensor](https://esphome.io/components/debug/)
- [ESPHome 2026.9.1](https://github.com/esphome/esphome/tree/2026.9.1)
- [GxEPD2 1.6.9](https://github.com/ZinggJM/GxEPD2/tree/1.6.9)

Dieses Projekt: GPL-3.0, siehe [LICENSE](LICENSE). Abhängigkeiten behalten
ihre eigenen Lizenzen; GxEPD2 wird regulär als Bibliothek heruntergeladen.
Keine echten Zugangsdaten, Home-Assistant-Konfigurationen oder Screenshots
werden in diesem Repository gespeichert.

## Automatischer Home-Assistant-Test

[Blueprint](homeassistant/epaper_test_blueprint.yaml) importieren und eine
Automation daraus erstellen. Button, Rendering-Binary-Sensor und Heap-Sensor
des Bilderrahmens auswählen. Die Automation besitzt keinen Zeitplan und
wird manuell über **Ausführen** gestartet. Sie wartet auf Rendering-Start und
-Ende, lässt nach jedem Durchlauf 30 Sekunden verstreichen und wiederholt
drei Mal. Bei fehlendem Start, Zeitüberschreitung oder nicht verfügbarem
Rendering-Status stoppt sie mit einer Benachrichtigung. Der Abschlussbericht
zeigt die von Home Assistant empfangenen Heap-Stichproben; er bewertet kein
sichtbares Bild und bestätigt weder Hibernate noch OTA. Während des Tests
den Displaybutton nicht zusätzlich manuell drücken.

Für Diagnose vorübergehend `logger.level: DEBUG` setzen.
