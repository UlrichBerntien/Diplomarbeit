# Programmpaket zur Spektrenauswertung

Das Programm basiert auf dem Programm IS30.C mit den Copyright-Vermerken
von D.Meiners, H.Wenz und J.Westheide.

## Hinweise zur Code-Erzeugung

Die C-Programme/Module werden kompiliert mit dem MS-C V5.10.

Dabei werden die Schalter
* /Ox                      (Optimierung auf Geschwindigkeit)
* /W3                      (höchste Warnstufe)
* /G2s                     (Code für i286, keine Stackkontrolle)
* /AL                      (Speichermodell Large).

Das Modul IRIS06.C darf nicht mit dem Schalter /Ox kompiliert werden,
weil der Optimierer Probleme mit den volatile Variablen hat. Das
Programm ist dann nicht lauffähig.

Programme, die auf einem IBM-XT kompatiblen PC laufen sollen, z.B.
das Programm PLOTOSZ.EXE, dürfen keinen i286 Code benutzen. Also bei
allen zugehörigen Modulen, z.B. PLOT.C, DIRGET.C und PLOTOSZ.C, den
Schalter /Gs statt /G2s benutzen.

Das Speichermodell LARGE muß bei allen Modulen benutzt werden, weil hier
keine andere Laufzeitbibliothek des C-Compilers zur Verfügung steht.
Die Bibliothek LLIBC7.LIB auf dem Rechner ist zerstört. Daher kann nur
die Bibliothek LLIBCE.LIB benutzt werden.

In einigen Include-Files des C-Compilers habe ich "char*" in
"const char*" umgesetzt (damit den ANSI-C Standart besser erfüllt).

Das Assembler-Modul UBPIXM.ASM wird mit MS-Assembler V4.0 übersetzt. Das
entstehende Objektmodul ist in die Bibliothek IRIS.LIB aufgenommen.

In der, bei der Bildaufnahmekarte mitgelieferte, Bibliothek DTISCLLI.LIB
wurden die Namen der Funktionen IS_* in Kleinbuchstaben umgesetzt.
In dem Headerfile ISDEFS.H wurden die Namen ebenfalls in Kleinbuchstaben
umgesetzt und die Prototypen der hier benutzten Funktionen wurden um ihre
Parameter ergänzt, um eine bessere Fehlerkontrolle durch den Compiler zu
ermöglichen.

Alle OBJ-Module IRIS*.OBJ werden in der Bibliothek IRISLIB.LIB zusammengefaßt.
Die Verwaltung der Bibliothek erfolgt mit ZORLIB.EXE, da das LIB von
Microsoft fehlt.

Alle OBJ-Module deren Source-Code im Verzeichnis COMMON stehen sind in
der Bibliothek COMMON\COMMON.LIB zusammengefaßt.

Das Linken von IRIS.EXE erfolgt mit dem MS-Link dabei die Schalter

* /NOI                     (Groß/Kleinschreibung beachten)
* /Stack:0x4000            (Runtimestack des Programms auf 0x4000 Bytes)

setzen.

Das Linken der anderen eigenständigen C-Programm erfolgt ebenfalls mit
MS-Link, aber nur mit dem Schalter

* /NOI                     (Groß/Kleinschreibung beachten).


## Dateien zur Programmerzeugung

Die folgenden Dateien enthalten allen Source-Code und sonstige Informationen
zum Erstellen des Programms IRIS.EXE und ein paar anderer Hilfsprogramme.


 Datei        |     Inhalt
------------- | --------------
 ISDEFS.H     |  Header-File, Schnittstelle zur Bibliothek DTISCLLI.LIB. Die Funktionsnamen wurden von mir in Kleinbuchstaben umgewandelt. Das File und die Bibliothek stammen aus dem Lieferumfang der Framegrabber-Karte.
 IRIS.H       |  Headerfile zu allen IRIS??.C und UBPIXM.ASM
 UBPIXM.ASM   |  Funktionen zum schnelleren Zugriff auf die Framegrabber-Karte Entstanden aus Analyse eines Teils des Bibliothek DTISCLLI.LIB.
 IRIS.C       |  Hauptprogramm, Benutzerführung durch eine Menü
 IRIS00.C     |  Eingaben von der Tastatur.
 IRIS01.C     |  "Fastpart": Beschriften, Splitten, Speichern, Expandieren. Und Bild umdrehen (Oberkante wird zu Unterkante), Bildspiegeln
 IRIS02.C     |  Speichern und Laden von Halbbildern und (Voll-)Bildern
 IRIS03.C     |  Spektren aus Bild gewinnen Die Grauwerte werden über mehrere Bildzeilen gemittelt, die Bildspalten-Auflösung (= Wellenlängenauflösung) bleibt erhalten.
 IRIS04.C     |  Spektren auswerten, Ergebnis ins Logfile schreiben. Spektren automatisch für alle Orte auswerten, ergibt Grafiken Ne(x) und Te(x). Diese werden in ein Plotfile geschrieben. Das Plotfile kann mit IRISNT.C ausgegeben werden.
 IRIS04.H     |  Header-File zu allen IRIS04?.C Modulen
 IRIS04A.C    |  Steuerfile IRIS_SP.TXT für die Spektrenauswertung einlesen
 IRIS04B.C    |  Berechnungen für die Spektrenauswertung, Halbwertsbreite, Verhältnis Linie/Kontinuum; Elektronen-Dichte und -Temperatur
 IRIS05.C     |  Helligkeitsverteilung über den Spalt. Die Grauwerte werden über mehrere Bildspalten gemittelt, die Bildzeilen-Auflösung (= Ortsauflösung) bleibt erhalten. Grauwerte eines 16 * 16 Feldes anzeigen und Zoom
 IRIS06.C     |  Datenübertragung DSO-Gould zum PC über die serielle Schnittstelle (RS-232) Benutzt das Modul RS232.C WICHTIG : Dieses Modul nicht mit eingeschalteter Optimierung kompilieren. Der Optimierer hat Probleme bei den volatile Variablen. Die empfangen Daten werden in Dateien *.OSZ gepeichert, diese können mit dem Programm PLOTOSZ.EXE ausgegeben werden.
 IRIS07.C     |  Schnittstelle zur DOS-Shell, Directory ausgeben
 IRIS08.C     |  Ansteuerung der Framegrabberkarte zur Bildaufnahme; Speichern der Bilder sofort nach der Aufnahme zur Sicherung
 IRIS09.C     |  Editieren einer LUT mit Darstellung dieser auf dem Bildschirm des PCs (benutzt die Grafikbibliothek des MS-C V51.0).
 STREDIT.C    |  Editieren eines einfachen Eingabefelds (stredit). Direktzugrif auf Bildschirmspeicher
 DIRGET.C     |  Zugriff auf Directories, Suchen nach Dateien
 IRIS.LNK     |  Response-File für MS-Link. Zusammenbauen von IRIS.EXE, alle IRIS*.OBJ Module werden in Bibliothek IRISLIB.LIB zusammengefaßt erwartet. Weiter werden die Bilbliotheken COMMON.LIB und DTISCLLI.LIB benötigt.
 IRISNT.C     |  Eigenständiges Programm, benutzt Modul PLOT.C. Ausgabe des Plotfiles, enthält Ne(x) und Te(x) Kurven auf einem Drucker HP Laserjet II. Das Plotfile wird vom Modul IRIS04.C erzeugt.
 IRISNT.H     |  Beschreibt den Aufbau der Files, die von IRISNT gelesen werden
 IRISNT.LNK   |  Response-File für MS-Link, erstellt Programm IRISNT.EXE
 PLOTOSZ.C    |  Eigenständiges Programm, benutzt Modul PLOT.C. Ausgabe der Daten vom DSO Gould auf Drucker HP-Laserjet oder NEC Pinwriter 2200 Die *.OSZ Files werden vom Modul IRIS06.C erzeugt.
 PLOTOSZ.LNK  |  Response-File für MS-Link. Zusammenbauen von PLOTOSZ.EXE aus PLOTOSZ.OBJ und COMMON.LIB
 OSZ2COL.C    |  Eigenständiges Programm Ausgabe der Daten vom DSO Gould in eine ASCII-Text Datei, Die Werte aus den Kanälen 1-4 (Timebase A) werden in vier      Spalten als Dezimalzahlen 0..255 ausgegeben. Die *.OSZ Files werden vom Modul IRIS06.C oder vom Programm GOULD.C erzeugt. 
 IRIS_DC.C    |  Eigenständiges Programm Bild der Kamera 1 wird auf Monitor durchgestellt und ein Fadenkreuz eingeblendet.
 IRIS_DC.LNK  |  Repsonse-File für MS-Link. Zusammenbau von IRIS_DC.EXE
 RS232.H      |  Headerfile zu RS232.C
 RS232.C      |  Behandlung der seriellen (RS-232) Schnittstelle. Einstellen der Schnittstellenparameter. Senden und Empfang von Zeichen (interrupt gesteuert).
 PLOT.H       |  Headerfile zu PLOT.C
 PLOT.C       |  Ausgabe von Plots auf einem HP-Laserjet II oder NEC Pinwriter 2200
 PRINTER.H    |  Headerfile zu PRINTER.C
 PRINTER.C    |  Steuercodes für Drucker HP-Laserjet und
              |  NEC Pinwriter 2200
 DRUCKEN.C    |  Steuerung des Drucks und der Archivierung der Logfiles.
 MACHE.C      |  Primitiver Batchprozessor mit Benutzerdialog


## Dateien zum Programmlauf

Die folgenden Dateien werden bei der Arbeit mit dem Programm IS30.EXE
benutzt, benötigt oder erzeugt.


 Datei        |     Inhalt
------------- | --------------
IRIS.EXE      |  Das lauffähige Programm
START.BAT     |  Batch-Datei zum Starten des Programms IRIS. Wird Parameter "z" angegeben, so wird IRIS.EXE aufgerufen, sonst die alte Version von IS30.EXE
IRIS_SP.TXT   |  Steuerdatei für das Programm IRIS.EXE. Ein einfaches ASCII-File mit jedem Editor zu bearbeiten. Enthält Einstellung, die das Auswerten der Spektren beeinflussen. z.B. Wellenlänge pro Bildpunkt, Tabellen über Stark-Verbreiterung.
 IRIS_OSZ.TXT |  Steuerdatei für das Programm IRIS.EXE. Ein einfachtes ASCII-File mit jedem Editor zu bearbeiten. Enthält Einstellungen, die die Übertragung der Daten DSO-Gould => PC beeinflussen. z.B. die Namen der Kanäle
 IS_DRIV.ADR  |  Die Datei wird in 'E:\' vom Programm C:DT_IRIS\IS_IRIS.EXE angelegt. (gestartet aus der AUTOEXEC.BAT). Das Programm IRIS.EXE kann (muß nicht immer) die Datei benötigen um die Adresse des IS-Drivers festzustellen.
 *.IMG        |  Halbbilder eines Bilds. Die Namen und Pfadangaben werden während des Programmlaufs vom Benutzer eingegeben.
 *.IM1        |  Beide Halbbilder eines Bilds.
 *.IM2        |  Üblich wird immer nur ein Halbbild (*.IMG) abgespeichert, der Benutzer kann aber auch beide Bilder speichern lassen.
 B:OUT.TXT    |  Das Logfile. Der Name des Files kann geändert werden, er ist in IRIS_SP.TXT festgelegt. ASCII-FILE mit den Ergebnissen der Spektrenauswertung. Enthält Steuercode-Sequenzenm, die den Drucker zwischen fetter und normaler Schrift umschalten, diese Steuercodes sind ebenfalls in IRIS_SP.TXT festgelegt.
 D:IRISNT.PLT |  Das Plotfile. Der Name des Files ist in IRIS_SP.TXT festgelegt und kann dort verändert werden. Das Plotfile enthält die Kurven Ne(x) und Te(x) erstellt vom Modul IRIS04.C. Das Plotfile kann mit IRISNT.EXE auf einem HP-Laserjet II ausgegeben werden. Ein Plotfile kann die Kurven zu mehreren Bildern aufnehmen.
 D:*.OSZ      |  In diesen Files werden die Daten von dem DSO Gould gepeichert. Der Name (ohne Extension) ist die Schussnummer. Die Erweiterung (*.OSZ) und der Pfad (D:) sind in IRIS_OSZ.TXT festgelegt und können dort geändert werden.
 LHARC.EXE    |  Komprimier- und Archivprogramm von H.Yoshizaki (Public-Domain)
 *.LZH        |  Archivdateien mit dem Programm LHARC.EXE erzeugt.
 <datum>.LZH  |  . In diesem Archivedateien werden die OSZ-Files eines  Schußtages zusammengefaßt
 B:LOGFLES.LZH|  . In dieser Archivdatei sind die Logfiles B:OUT.TXT zusammengefaßt.
 MACHE.EXE    |  primitiver Batchprozessor, erlaubt Benutzerdialog
 ENDE.MAT     |  Scriptfile für MACHE.EXE, Aufräumen nach einem Schußtag: archivieren und sichern der OSZ-Files.
 PLOTOSZ.EXE  |  Programm zum Ausgeben der *.OSZ Files auf einen HP-Laserjet II oder kompatiblem Drucker.
 OSZ2COL.EXE  |  Programm liest die Werte der 4 Kanäle aus einem *.OSZ File und schreib sie als Dezimalzahlen spaltenweise in ein ASCII Text File.
 IRISNT.EXE   |  Programm zum Ausgeben des Plotfiles mit den Kurven Ne(x) und Te(x) auf einen HP-Laserjet II.
 FP.COM       |  Programm zum Listen von ASCII-Files auf HP-Laserjet oder Epson FX-80.
 DRUCKEN.COM  |  Steuert das Ausdrucken und Archivieren der Logfiles, benutzt dabei die Programme FP.COM und LHARC.EXE
          