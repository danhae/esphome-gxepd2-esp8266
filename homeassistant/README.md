# Drei automatische Test-Refreshs

1. Die vollständige bilderrahmen.yaml aus Release v1.0.2 installieren.
   Für den Diagnosetest `logger.level: DEBUG` verwenden.
2. Home Assistant muss die drei Entitäten anzeigen: E-Paper Testbild anzeigen,
   E-Paper Rendering (Binary-Sensor) und E-Paper Freier Heap.
3. Unter Einstellungen → Automationen & Szenen → Blueprints importieren:
   https://github.com/danhae/esphome-gxepd2-esp8266/blob/v1.0.2/homeassistant/epaper_test_blueprint.yaml
4. Eine Automation aus dem Blueprint erstellen und die drei Entitäten
   dieses Bilderrahmens auswählen. Speichern.
5. Die Automation manuell mit Ausführen starten. Währenddessen nicht
   zusätzlich den Displaybutton drücken. ESPHome-Logs geöffnet lassen.

Die Automation hat keinen Zeitplan und startet nicht bei jedem HA-Neustart.
Sie führt drei Refreshs aus, wartet auf das gemeldete Rendering-Ende und
anschließend jeweils 30 Sekunden. Bei Start-/Ende-Timeout oder nicht verfügbarem
Rendering-Status bricht sie ab. Einzelne Betriebssystem-/Netzwerkverzögerungen
können auch einen Testabbruch bewirken; der Trace zeigt den betreffenden Schritt.

Die Abschlussbenachrichtigung berichtet die drei Heap-Stichproben.
Sie bestätigt keine Bildqualität, keinen gemessenen Hibernate-Zustand und
kein erfolgreiches OTA. Auf dem echten Gerät wurde dieser Blueprint hier
nicht ausgeführt. Schema-Prüfung: Home Assistant 2026.10.1 erfolgreich.

Die Automation kann über den laufenden Trace gestoppt werden. Ein bereits
an die Firmware gesendeter Refresh wird dadurch nicht abgebrochen; die
weiteren Durchläufe werden unterbunden.
