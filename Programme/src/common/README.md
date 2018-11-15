# COMMON

In diesem Verzeichnis sind allgemein verwendete Module und die Firmware vom
Hersteller der Framegrabber-Karte zusammengefasst.

Name         | Inhalt
------------ | ------
ISDEFS.H     | Header-File zur Firmware. Ergänzt mit den Typen der Funktionsparametern für striktes Prototyping
ISERRS.H     | Fehlercodes der Firmware
DTISCLLI.LIB | Bibliothek mit Firmware zur Ansteuerung der Framegrabber-Karte
COMMON.H     | Header-File für die Module DIRGET.C und STREDIT.C
DIRGET.C     | Zugriff auf Verzeichnisse, Suchen nach Dateien
STREDIT.C    | Editieren in einem einfachen Eingabefeld, Direktzugriff auf Bildschirmspeicher
RS232.C      | Zugriff auf RS-232 Schnittstelle, Empfangen und Senden erfolgt Interrupt-gesteuert
RS232.H      | Header-File zu RS232.C
PLOT.C       | Ausgabe von Plots auf einem HP-Laserjet II kompatiblen Drucker
PLOT.H       | Header-File zu PLOT.C
GOULD1.C     | Datenaustausch zwischen einem PC und einem Digitalspeicheroszilloskop Gould 4070 über die serielle Schnittstelle
