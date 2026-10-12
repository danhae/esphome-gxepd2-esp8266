# E-Ink Dashboard verbinden — v1.1.0-beta.2

Bildgestaltung bleibt vollständig bei der installierten Integration
`cryptomilk/hass-eink-dashboard`. Der Adapter lädt nur deren PNG und wandelt
es in die vorhandene EPD1-Übertragung um. Kein eigener Dashboard-Renderer.

## Home Assistant: zwei Dateien

1. `tools/prepare_image.py` nach `/config/epaper/prepare_image.py` kopieren.
2. `homeassistant/eink_dashboard_package.yaml` nach
   `/config/packages/epaper_eink_dashboard.yaml` kopieren.
3. HA-Packages müssen in deinem vorhandenen `homeassistant:`-Abschnitt
   eingebunden sein:

```yaml
homeassistant:
  packages: !include_dir_named packages
```

Bestehende Schlüssel zusammenführen. Konfiguration prüfen und HA neu starten.
Das Package enthält den Helper **E-Paper Dashboard PNG-URL**, den Konverter,
das Script **E-Paper — E-Ink Dashboard aktualisieren** und eine Automation,
die alle fünf Minuten die Bilddatei vorbereitet. Diese Automation löst
keinen physischen Display-Refresh aus.

## Dashboard und Bildquelle

In E-Ink Dashboard ein Gerät mit Custom-Auflösung 800×480, Querformat,
Pull-only und zwei Graustufen einrichten und Widgets speichern. Seine
Image-Entität muss anschließend in HA sichtbar sein.

Das neue Script über die HA-Oberfläche ausführen. Dabei auswählen:

- **Bild des E-Ink Dashboards:** die Image-Entität der Integration.
- **E-Paper Bild aktualisieren:** den Button deines ESPHome-Bilderrahmens.

Das Script ermittelt die Dashboard-ID aus der ausgewählten Image-Entität
(`config_entry_id`) und speichert die Quelle selbst im Helper. Es verwendet
zunächst `http://127.0.0.1:8123/api/eink_dashboard/<ID>/image.png` innerhalb
von HA Core. Bei SSL im Core oder anderem Port den gespeicherten Helper auf
die tatsächlich erreichbare PNG-Adresse ändern; die Hintergrund-Konvertierung
verwendet diese Adresse. Der Script-Aufruf setzt die Standardadresse erneut.
Bei solchen abweichenden HTTP-Einstellungen auch die `input_text.set_value`-
Zeile des Packages entsprechend anpassen.

Die Integration stellt diesen PNG-Endpunkt laut ihrer Dokumentation ohne
Authentifizierung bereit. Der Adapter benötigt keinen HA-Zugangstoken.
Quelle: https://github.com/cryptomilk/hass-eink-dashboard#device-setup

## ESPHome

Vollständige `bilderrahmen.yaml` verwenden und selbst kompilieren/installieren.
Die C++-Display-Komponente ist unverändert gegenüber beta.1. Deine geprüften
Pins und Displayklasse bleiben. Keine Firmware wurde für diese Version gebaut.

In HA **E-Paper Bild-URL** auf die von deinem ESP8266 erreichbare Adresse
`http://<HA-Host>:8123/local/epaper/frame.epd` setzen. Das ist die konvertierte
Datei, nicht die Dashboard-PNG-Adresse. Host/Port müssen deine tatsächliche
lokale HTTP-Adresse sein.

Dann das neue HA-Script einmal ausführen. Es lädt das Dashboard-PNG, schreibt
atomar `/config/www/epaper/frame.epd`, erzeugt `/config/www/epaper/preview.png`
und betätigt erst nach erfolgreicher Konvertierung den ausgewählten Button.
Die Vorschau wird im Browser unter `/local/epaper/preview.png` angezeigt.

Nach dem ersten Script-Aufruf erzeugt die Hintergrund-Automation alle fünf
Minuten eine aktuelle Datei. Den physischen Refresh kannst du danach auch
über den bekannten ESPHome-Bildbutton auslösen. Für einen automatischen
Display-Zeitplan eine HA-Automation auf das neue Script mit den gewählten
Entity-IDs anlegen; es gibt keine geratenen IDs im Package.

## Validierung und Grenzen

Home-Assistant-2026.10.1-Schemas und Template-Funktionen sind validiert. Die
PNG-URL wird Base64-kodiert an die Shell übergeben; Quotes, Dollarzeichen und
Backticks gelangen nicht als ausführbarer Shelltext in den Befehl.
Downloads sind auf 8 MiB begrenzt und verwenden einen Socket-Timeout von 15s.
Konvertierungsfehler belassen die bisherige EPD1-Datei und verhindern den
Script-Buttondruck. Die HA-Integration rendert selbst im konfigurierten
Intervall; der Adapter kann nur das aktuell bereitgestellte PNG übernehmen.

Es wurde keine Verbindung zu deiner privaten HA-Installation hergestellt.
Ein live erfolgreicher PNG-Download, dein ESP8266-Build und die Anzeige des
Dashboard-Bilds sind weiterhin am Gerät zu prüfen. Native API und OTA bleiben
verschlüsselt; der ESP-Bilddownload verwendet lokales HTTP wie bisher.
