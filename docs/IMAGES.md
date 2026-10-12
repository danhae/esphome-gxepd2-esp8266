# Eigene Bilder anzeigen — v1.1.0-beta.1

Diese Erweiterung ist eine Vorschauversion. Kein neuer ESP8266-Firmwarebuild
wurde vom Agenten oder in CI gestartet: der Benutzer kompiliert selbst.
Schema/Quellgenerierung, Konverter und simulierte Übertragung sind getestet;
Übertragung und Anzeige am realen ESP8266 stehen aus.

## Erster Test mit fertiger Datei

1. Die vollständige `bilderrahmen.yaml` aus diesem Release in ESPHome einsetzen.
   Bestehende WLAN- und API-Secrets beibehalten. Selbst kompilieren/installieren.
2. `images/demo.epd` aus diesem Repository herunterladen und nach
   `/config/www/epaper/frame.epd` in Home Assistant kopieren. Falls `www` neu
   angelegt wurde, Home Assistant gemäß HTTP-Dokumentation neu starten.
3. In einem Browser im lokalen Netz prüfen, dass
   `http://<dein-HA-Host>:8123/local/epaper/frame.epd` die Datei liefert.
   Host und Port sind deine tatsächliche HTTP-Adresse. Bei SSL-only-HA
   funktioniert diese HTTP-Variante nicht; sie benötigt einen erreichbaren
   lokalen HTTP-Dateiserver mit Content-Length. Keine HTTPS-Umleitung.
4. Beim ESPHome-Gerät in Home Assistant **E-Paper Bild-URL** auf diese
   tatsächliche HTTP-Adresse setzen. Anschließend **E-Paper Bild aktualisieren**
   drücken. Es ist kein automatischer Refresh beim Boot vorgesehen.
5. Erwartetes Log: `48000 Bildbytes und CRC32 geprueft; Display-Refresh`,
   anschließend `Renderdurchlauf beendet; Hibernate gesendet`.

Der bekannte Testbildbutton bleibt als Hardwarediagnose erhalten. Die
Rendering-Sperre gilt gemeinsam für Testbilder und eigene Bilder.
`/local`-Dateien sind ohne Home-Assistant-Login zugänglich. Dort nur das
beabsichtigte Anzeigebild ablegen, keine Secrets oder private Konfigurationsdateien.

## Eigene PNG-/JPEG-/andere Pillow-Bilder umwandeln

Auf einem Rechner mit Python und Pillow:

```sh
python -m pip install -r tools/requirements.txt
python tools/prepare_image.py --input mein-bild.png --output frame.epd --preview preview.png
```

`frame.epd` nach `/config/www/epaper/frame.epd` kopieren und Bild aktualisieren.
Du kannst denselben Dateinamen mit neuen Inhalten ersetzen; für jedes neue
Bild ist kein Firmwarebuild notwendig. Der Konverter schreibt atomar, damit
HA keine halb geschriebene Ausgabe ausliefert.

Standard: Seitenverhältnis erhalten, weißer Rand, Floyd-Steinberg-Dithering.
Weitere Optionen: `--fit cover` (ausschneiden), `--fit stretch`, `--no-dither`
(für harte Text/Grafik-Kanten), `--invert`, `--contrast 1.2`.
Transparenz wird auf Weiß gelegt; EXIF-Ausrichtung wird berücksichtigt.
SVG/PDF zuerst als PNG exportieren. Fotos bleiben auf dem Panel monochrom.

## Bildaufbereitung direkt in Home Assistant

Optional `tools/prepare_image.py` nach `/config/epaper/prepare_image.py` kopieren
und das gewünschte PNG nach `/config/www/epaper/input.png` legen.
Das Package `homeassistant/epaper_images_package.yaml` enthält einen festen
Shell-Command und ein Script zum Konvertieren und anschließenden Buttondruck.
Es nimmt keine Shell-Befehle oder Dateipfade aus Texteingaben entgegen.

Wenn HA-Packages aktiviert sind, die Datei nach
`/config/packages/epaper_images.yaml` kopieren und die Konfiguration prüfen,
danach Home Assistant neu starten. Beispiel für die einmalige Packages-
Einbindung in deinem bestehenden `homeassistant:`-Abschnitt:

```yaml
homeassistant:
  packages: !include_dir_named packages
```

Bestehende Top-Level-Schlüssel zusammenführen, nicht doppelt hinzufügen.
Ohne Packages nur die `shell_command:`- und `script:`-Einträge in die jeweiligen
bestehenden Abschnitte übernehmen. Die Standard-HA-Core-Installation enthält
Python/Pillow; bei abweichenden Installationen muss es im Core-Container
verfügbar sein. Der Shell-Command läuft im Home-Assistant-Core-Container,
nicht im ESPHome-Add-on. Der Script-Aufruf verlangt die Auswahl des tatsächlichen
Buttons **E-Paper Bild aktualisieren**. Die Bild-URL vorher einmal einstellen.
Das Script startet den Refresh, meldet aber nicht dessen physischen Erfolg.

## Speicher, Protokoll und Fehlerbehandlung

EPD1: 16 Byte Header, exakt 48000 Nutzbytes, insgesamt **48016 Byte**.
Header: `EPD1`, uint16 Breite 800, uint16 Höhe 480, uint32 Länge 48000,
uint32 CRC32 der Nutzbytes, alle Zahlen little-endian. Nutzdaten: zeilenweise,
100 Byte/Zeile, MSB links, Bit 1 weiß, Bit 0 schwarz.

Die feste GxEPD2_BW<GxEPD2_750_T7, 16>-Instanz bleibt erhalten (1600 Byte).
Der HTTP-Pfad braucht zusätzlich einen festen 1600-Byte-Streifenpuffer:
**3200 Byte dedizierte Pixel-/Transferpuffer**, plus HTTP/TCP-Metadaten und
kleine Verwaltungsfelder. Kein vollständiges Bild im RAM, keine Flash-
Bilddatei, kein PNG/JPEG-Decoder auf dem Gerät. Der Download schreibt 30
Streifen in den Displaycontroller und löst erst nach vollständiger Länge und
CRC32 einen Full-Refresh aus. Fehlerhafte Downloads werden ohne Refresh
verworfen; der nächste Versuch initialisiert den Controller neu.

HTTP 200 und Content-Length 48016 sind erforderlich. Chunked-Responses,
Redirects, Login-Seiten und direkte PNG/JPEG-URLs werden abgewiesen.
HTTPS ist absichtlich deaktiviert, um TLS-Speicher auf ESP8266 zu vermeiden.
Die native Home-Assistant-API und OTA bleiben verschlüsselt. Der Bilddownload
verwendet lokales HTTP; CRC32 prüft Übertragungsfehler, nicht die Authentizität.

10 Sekunden ohne Daten führen zum Abbruch; der gesamte Auftrag hat ein
120-Sekunden-Zeitlimit zwischen Bibliotheksaufrufen. HTTP-Verbindungsaufbau
(max. konfigurierter Timeout 5s) und physischer Refresh bleiben synchron und
können die API kurz verzögern. Ein hängender Download wird beendet; Rendering-
Status und TCP-Ressourcen werden freigegeben. Unter 16000 Byte freiem Heap
oder 8192 Byte größtem Block wird kein neuer HTTP-Auftrag angenommen.

Die direkte GxEPD2-Controller-Streifenübertragung ist neu; die bisherige
Testbilddarstellung bleibt bei firstPage()/nextPage(). Ob eigene Bilder auf
diesem Panel korrekt erscheinen, muss am Gerät getestet werden.

Quellen: https://esphome.io/components/http_request/ und
https://www.home-assistant.io/integrations/http/#hosting-files
