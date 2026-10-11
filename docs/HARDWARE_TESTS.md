# Offene Hardware-Abnahme

Alle folgenden Punkte stehen aus. Ein erfolgreicher Compilerlauf bestätigt
weder die Verkabelung noch den sichtbaren Refresh dieser ESPHome-Firmware.

- [ ] Mehrfacher Kaltstart ohne Safe Mode; GPIO15 beim Reset LOW.
- [ ] WLAN und verschlüsselte Home-Assistant-API erreichbar.
- [ ] Button zeichnet weißen Hintergrund, Außenrahmen, Rechteck,
      Schachbrett und Text EPAPER TEST.
- [ ] Anschließend Hibernate; Bild bleibt sichtbar.
- [ ] Weitere verarbeitete Button-Aufrufe während Rendering werden verworfen.
- [ ] Erneuter Refresh nach Hibernate funktioniert.
- [ ] Freier Heap bleibt vor/nach mehreren Refreshs ausreichend und stabil.
- [ ] Keine Watchdog-Resets; API kehrt nach Refresh zurück.
- [ ] Verschlüsseltes OTA funktioniert nach mehreren Aktualisierungen.
- [ ] Ein Display-/BUSY-Fehler führt zu begrenzter Wartezeit und Rückkehr.

Bei Problemen zuerst WARN-/DEBUG-Logs und Heap-Werte erfassen. BUSY ist beim
T7 aktiv LOW. Nicht auf Verdacht eine andere Displayklasse oder Pinbelegung
wählen: GxEPD2_750_T7 und die Pins wurden im Arduino-Test bereits bestätigt.
