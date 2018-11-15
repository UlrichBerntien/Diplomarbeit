# Konvertierungsprogramme für Bilder

Konvertierung | Programm
------------- | -------------
IMG  > BMP    | img2bmp.exe
BMP  > IMG    | bmp2img.exe
IMG  > PCX    | img2pcx.exe
PCX  > IMG    | pcx2img.exe
IMG  > GIF    | img2gif.exe
GIF  > IMG    | gif2img.exe
IMG  > TIFF   | img2tiff.exe
TIFF > IMG    | tiff2img.exe


# Hinweise zu Coreldraw:

Coreldraw importiert nicht alle Grafikformate in jeder Form.

BMP-Format : nur unkomprimiert, also Schalter "-1" immer setzen.

PCX-Format : nur ohne Farbtabelle und nicht komprimiert, also die
Schalter "-1" und "-f" nicht setzen.

TIF-Format : nur die Version 5.0 wird gelesen, also den Schalter
  "-d" nicht setzen.
  Die Streifen dürfen nicht zu groß sein (64 KByte-Grenze ?), also
  ohne die Schalter "-2" odser "-4" immer mit 8 Streifen pro Bild
  arbeiten.

# Literatur zu Grafikformaten:

Die Artikelreihe wurde hauptsächlich als Quelle verwendet:

-  Thomas W. Lipp : Grafikkonvertierung unter Windows
-  Microsoft System Journal, Hefte 1/1993 bis 4/1993

Diese Artikel sind aber leider nicht ganz fehlerfrei und vollständig.
Auch werden wichtige Teile nur kurz beschreiben, andere unnötig lang.

Weiter Artikel über Grafikformate finden sich in der mc:

-  "Grafik im Griff, dem TIFF Dateiformat aufs Bit geschaut" mc 10/1990
-  "Grafik mit Format, dem PCX Format aufs Bit gefühlt" mc 12/1990
-  "Heiße Pixel - GIF Graphics Interchange Format" mc 5/1991
-  "Das Bitmap-Format entschlüsselt" mc 8/1993 und mc 9/1993

Diese Artikel sind in der Regel noch knapper als die oben genannten, aber oft eine gute Ergänzung.

# Dateien

Datei |	Inhalt
----- | ------
MAKEFILE | steuert die Erstellung der Programme mit dem Zortech C Compiler Version 2.1 (Zortech C++ V2.1)
GENIMG.C | Programm zur Erstellung von Testbildern im IMG-Format
IMG2X.C  | Zusammenfassung von Funktionen, die in allen Umwandlungsprogrammen von IMG-Format in x-Format benötigt werden, enthält auch die main() Funktion.
IMG2X.H  | Headerfile zu IMG2X.C und zu allen 2x.C Modulen. Definiert die Software-Schnittstelle zwischen dem IMG2X.C und den 2x.C Modulen.
 X2IMG.C | Zusammenfassung von Funktionen, die in allen Umwandlungsprogrammen von x-Format in IMG-Format benötigt werden, enthält auch die main() Funktion.
 X2IMG.H | Headerfile zu X2IMG.C und zu allen x2.C Modulen. Definiert die Software-Schnittstelle zwischen dem X2IMG.C und den x2.C Modulen.
 BMP.H	 | Definitionen für das BMP-Format, Aufbau des File-Headers, der Farbtabelle
 2BMP.C  | Funktionen und Daten für die Umwandlung vom IMG ins BMP-Format von Windows 3.X
 BMP2.C  | Funktionen und Daten für die Umwandlung vom BMP-Format in das IMG-Format
 PCX.H	 | Definitionen für das PCX-Format, Aufbau des File-Headers, der Farbtabelle
 2PCX.C  | Funktionen und Daten für die Umwandlung vom IMG ins PCX-Format Version 2.8 mit Farbtabelle oder in Version Version 3.0 ohne Farbtabelle von ZSoft (Paintbrush)
 PCX2.C  | Funktionen und Daten für die Umwandlung vom PCX-Format in das IMG-Format
 GIF.H	 | Definitionen für das GIF-Format 2GIF.C. Funktionen und Daten für die Umwandlung vom IMG ins GIF-Format Version 89a von CompuServe
 GIF2.C  | Funktionen und Daten für die Umwandlung vom GIF ins IMG-Format
 TIFF.H  | Definitionen für das TIF-Format, Konstanten, Strukturen
 TIFF2.C | Funktionen und Daten für die Umwandlung vom TIF-Format ins IMG-Format. Aufgrund der vielfältigen Möglichkeiten Bilder in TIFF Dateien zu speichern kann nur ein kleiner Teil unterstützt werden.
 2TIFF.C | Funktionen und Daten für die Umwandlung vom IMG ins TIF-Format Version 5.0 oder in Version 6.0 (mit Differencing Predictor-Verfahren) von Aldus

# Hinweise zur Portierung

Der benutzte C-Compiler erzeugt 16Bit-Code, die Datentypen wurden
so gew„hlt, daá auch beim šbersetzen mit einem 32Bit-Compiler keine
Probleme mit den Dateiformaten entstehen sollten.

Bei einigen Dateiformaten ist eine Ausrichtung auf Byte-Grenzen
erforderlich. Diese Ausrichtung wurde in den Datenstrukturen
durch die Einschlieáung in die Pragma-Anweisungen

  #pragma ZTC align 1
  #pragma ZTC align

erreicht.

Auáerhalb diese Pragma-Anweisungen werden die Komponenten vom
Compiler auf ihre natürlichen Grenzen ausgerichtet.
Unabhängig von diesen Pragma-Anweisungen besitzen die Strukturen
immer eine Länge, die ein vielfaches von 2 Bytes ist.

Im wesentlichen wurden nur ANSI-C Funktionen benutzt. Aber zum
schnelleren Filezugriff wurden spezeille auf DOS zugeschnittene
Funktionen benutzt.

Benutze Funktionen aus der Laufzeit-Bibliothek des Zortech C++ V2.1
Compilers, die keine ANSI-Funktionen sind im folgenden beschrieben.

```C
/*
**  Prüfen einer Datei oder eines Pfades auf Zugriffsmöglichkeit
**  path   : Names des Files oder des Pfades
**  mode   : mögliche Werte, auch mit | kombinierbar:
**	     F_OK   prüfen auf Existens
**	     X_OK   prüfen auf Ausführungserlaubnis
**	     W_OK   prüfen auf Schreiberlaubnis
**	     R_OK   prüfen auf Leseerlaubnis
**  return : wenn erlaubt 0, sonst -1
*/
int access ( const char* path, int mode );

/*
**  Schliessen einer Datei
**  fd	   : der File-Handle der Datei
**  return : wenn erfolgreich 0, sonst -1
*/
int close ( int fd );

/*
**  Anlegen einer Datei
**  name      : Name der Datei (mit Pfad)
**  attribute : möglich sind die Atrribute FA_RDONLY, FA_HIDDEN, FA_SYSTEM,
**		FA_LABEL, FA_DIREC, FA_ARCH auch mit | kombinierbar.
**  return    : File-Handle der angelegten Datei, oder -1 im Fehlerfall
*/
int dos_creat ( const char* name, unsigned attribute );

/*
**  Öffnen einer existierenden Datei
**  name   : Name der zu öffnen Datei (mit Pfad)
**  mode   : möglich sind O_RDONLY, O_WRONLY oder O_RDWR
**  return : File-Handle der geöffneten Datei, oder -1 im Fehlerfall
*/
int dos_open ( const char* name, int mode );

/*
**  Bestimmt die Größe einer geöffneten Datei in Bytes
**  fd	   : der File-Handle der Datei
**  return : Größe der Datei, oder -1L im Fehlerfall
*/
long filelength ( int fd );

/*
**  Lesen eines Zeichens von der Konsole ohne ein Echo auszugeben
**  return : ASCII Wert des gelesen Zeichens
*/
int getch ( void );

/*
**  Bewegen des Schreib/Lese-Zeigers innerhalb einer Datei
**  fd	   : File-Handle der Datei
**  offset : um diesen Wert in Bytes verschieben
**  mode   : die Neupositionierung erfolgt bezüglich
**	     SEEK_SET	 Anfang der Datei
**	     SEEK_CUR	 aktuelle Position des Zeigers
**	     SEEK_END	 Dateiende
**  return : neue Position des Schreib/Lese-Zeigers relativ zum
**	     Dateianfang, oder -1L im Fehlerfall
*/
long lseek ( int fd, long offset, int mode );

/*
**  Lese aus einer geöffneten Datei
**  fd	   : der File-Handle der Datei
**  buffer : dorthin werden die gelesen Bytes geschrieben
**  length : Anzahl der Bytes, die gelesen werden sollen
**  return : die Anzahl der gelesen Bytes, oder -1 im Fehlerfall
*/
int write ( int fd, void* buffer, size_t length );
```
