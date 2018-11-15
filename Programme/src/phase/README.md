# PHASE

Programm zur Auswertung der Signale eines Interferomters mit
2-Phasendetektion.

Das Auswertungsprogramm ist von den Programmen zur Ausgabe der
Plots getrennt, weil dies auf verschiedenen PCs erfolgt.

Für die Compilierung der C-Programm können die Compiler Microsoft C V5.10,
Zortech C V2.1 oder Turbo C V2.0 verwendet werden.
Nur das Modul PHASE03.C benötigt für die Grafikausgabe auf den Monitor
die Grafikbibliothek des Microsoft-Compilers.
Das Assemblerprogramm kann mit dem Microsoft Macro Assembler V4.00
übersetzt werden.

Aus dem COMMON-Verzeichnis werden noch die Dateien

RS232.C   |  Ansteuerung der RS-232 Schnittstelle des PCs
RS232.H   |  Headerfile zu RS232.C
PLOT.C    |  Plots auf HP-Laserjet II
PLOT.H    |  Headerfile zu PLOT.C
STREDIT.C |  Editieren von Strings auf dem Bildschirm
DIRGET.C  |  Durchsuchen von Verzeichnissen nach Dateien
COMMON.H  |  Headerfile zu STREDIT und DIRGET

benutzt. Die Objektcode-Module dieser Dateien sind in der Bibliothek
COMMON\COMMON.LIB zusammengefaßt.

Verwendete/erzeugte - Files

 * OSZ-Files: Daten vom Digitalspeicheroszilloskop Gould 4074, einfaches ASCII-File, Daten im Format der DSO Gould.
 * PHD-Files: Daten aus dem Auswertungsprogramm PHASE2.EXE zum Speichern der Ergebnisse, kann vom Auswertungsprogramm wieder gelesen werden.
 * Plotfiles Vom Auswertungsprogramm erzeugt, für die Ausgabe von Plots über TGPLOT.EXE oder TGTEX.EXE auf HP-Laserjet II oder in TeX-Source.

  Datei          |        Inhalt
---------------- | -------------------------------------------------------
 INT64.ASM       |  64bit Arithmetik auf einem 16bit Prozessor
 INT64.H         |  Headerfile zu INT64.ASM
 LST2TEX.C       |  Umwandlung Programm-Source in Form für den Ausdruck mit dem TeX-Drucksatzprogramm
 PHASE. H        |  Headerfile für alle PHASE??.C Files
 PHASE. LIB      |  Libary mit allen PHASE?? Objectmodule und INT64.OBJ
 PHASE.LNK       |  Responsefile für Linker, erzeugt PHASE2.EXE
 PHASE00.C       |  elementare Funktionen für die Benutzerschnittstelle
 PHASE01.C       |  Datenübertragung PC <-> DSO Gould via RS-232
 PHASE02.C       |  Erzeugen von Plotfiles
 PHASE03.C       |  Plots auf PC-Monitor
 PHASE04.C       |  Steuerung aller Plotausgaben
 PHASE05.C       |  Berechnung der Dichte aus den 2-Phasensignalen
 PHASE06.C       |  PHD-File lesen/schreiben und OSZ-File lesen
 PHASE1.C        |  erste Testversion mit 1-Phasendetektion
 PHASE2.C        |  Hauptprogramm der Auswertung des Interferometers mit 2-Phasendetektion
 TGP.H           |  Headerfile
 TG.H            |  Headerfile für alle TG??.H Module
 TG00.C          |  lesen aus Plotfile mit Fehlerbehandlung
 TG01.C          |  Schreiben in Ausagbefile/Drucker mit Fehlerbehandlung
 TGPLOT.C        |  Ausgabe von Plotfiles auf dem HP-Laserjet II
 TGTEX.C         |  Einfügen von Plotfiles in TeX-Sourcefiles. Aber PC-TeX hat Speicherplatzprobleme mit den vielen \put und \line Befehlen
