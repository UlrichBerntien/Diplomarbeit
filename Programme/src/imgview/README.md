# Viewer für das IMG-Format auf EGA,VGA, VESA und Tseng-Karten

Literatur zu EGA,VGA, VESA und Tseng-Videoadapter:

Tischer : PC intern 3.0, Data Becker, Düsseldorf 1992, ISBN 3-89011-591-8

VGADOC2.ZIP aus dem Internet

Handbücher der Firma Tseng

# Dateien:

Datei        | Inhalt
------------ | ---------
MAKEFILE.ZTC | Steuert das Erstellen des Programms imgview.exe mit dem Zortech C V2.1 Compiler und dem MS-MASM-Assembler
MAKEFILE.MSC | Steuert das Erstellen mit dem Microsoft C V5.1 Compiler und dem MS-MASM-Assembler
IMGVIEW.H    | Interface Definitionen zwischen den Modulen
IMGVIEW.C    | Hauptprogramm mit dem Laden der Halb- oder Vollbilder
EGA.C	     | Zugriff auf eine EGA-Karte
VGA.C	     | Zugriff auf eine VGA-Karte
SVGA.C       | Zugriff auf eine SVGA-Karte über das VESA-Bios
TSENG.C      | Zugriff auf eine TSENG-Karte (ET3000,ET4000,ET4000-W32)
VIEWASM.ASM  | Hilfsroutinen in Assembler zur Beschleunigung

