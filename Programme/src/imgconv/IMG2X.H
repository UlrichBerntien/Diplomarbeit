/*
**  Headerfile für das Modul IMG2X.C
**  und alle Module in die konvertiert wird 2BMP.C, 2PCX.C,
**  2GIFF.C und 2TIFF.C
*/

/*
**  Elementare Datentypen
*/
typedef unsigned char  BYTE;             /*  8 Bit ohne Vorzeichen */
typedef unsigned short WORD;             /* 16 Bit ohne Vorzeichen */
typedef unsigned long  DWORD;            /* 32 Bit ohne Vorzeichen */
typedef long           LONG;             /* 32 Bit mit Vorzeichen  */

/**** in img2x.c ****************************************************/

/*
**  Mögliche Arten der Umwandlung Grauwert x in RGB-Wert (y,y,y)
*/
typedef enum img_skalierungTAG
  {
  _img_normal, _img_minmax, _img_stufen
  }
  img_skalierung;

/*
**  Fehlerausgabe und Programm beenden
**  format : Format für fprintf
*/
void error ( const char* format, ... );

/*
**  Fehlermeldung und Programmende, weil ein interner Fehler aufgetreten
**  fktname : Name der Funktion
**  msg     : Fehlermeldung
*/
void internerror ( const char* fktname, const char* msg );

/*
**  wie malloc, aber mit Fehlermeldung
**  size   : Größe des benötigten Speichers auf dem Heap in Bytes
**  return : Zeiger auf den Speicherblock (nie NULL)
*/
void* Malloc ( size_t size );

/*
**  wie lseek, aber mit Fehlerbehandlung
**  file     : Seek-Operation auf dieses File ausführen
**  position : R/W-Pointer auf diese Position (von Fileanfang) setzen
**  filename : Name des Files für Fehlermeldung
*/
void Lseek ( int file, long position, const char* filename );

/*
**  Konstanten des IMG-Formats
*/
#define img_width     512   /* Breite des Bilds in Pixel            */
#define img_width_mm   95   /* Breite in mm (mit SONY Videoprinter) */
#define img_height_mm  63   /* Höhe in mm (mit SONY Videoprinter)   */

/*
**  Höhe des Bild in Zeilen 256 oder 512
**  Die Entscheidung wird über die Dateigröße beim Öffnen des
**  IMG-Files getroffen.
*/
extern int img_height;

/*
**  Konstanten für das IMG-Modul
*/
#define img_bufferlines 80  /* Zeilen im Buffer */

/*
**  Buffer für das Lesen aus dem IMG-File
*/
extern BYTE* img_buffer;

/*
**  Anzahl der Bytes im Buffer
*/
extern size_t img_bufferused;

/*
**  nächste Leseposition aus dem Buffer
*/
extern size_t img_readpointer;

/*
**  Anzahl der noch zu lesenden Zeilen aus dem IMG File
*/
extern int img_zeile;

/*
**  Filehandle für Zugriffe auf das File (read binary)
*/
extern int img_file;

/*
**  Fehlermeldung und Programmende bei Problemen mit dem IMG-File
**  action : bei dieser Aktion ist der Fehler aufgetreten
*/
void img_error ( const char* action );

/*
**  Seek-Operation auf dem img_file ausführen mit Fehlerkontrolle
**  offset : Read/Write Pointer auf diese Position setzen
*/
void img_seek ( long offset );

/*
**  Die Grauwerte im Buffer 'img_buffer' in der Tabelle 'img_used' registrieren
**  und ggf. glätten
*/
void img_registriere ( void );

/*
**  Ermittlung der Tabelle Grauwert x - RGB-Wert (y,y,y)
**  art    : Art der Umwandlung Grauwert - RGB-Wert
**  return : die Tabelle der Werte y indiziert durch die Grauwerte x
**           (free nicht vergessen)
*/
BYTE* img_getgrayscale ( img_skalierung art );

/*
**  Name des OUT-Files für Meldungen
*/
extern const char* out_filename;

/*
**  Schreiben in den 'out_buffer'
**  size   : Anzahl der zu schreibenden Bytes
**  buffer : der Buffer mit den zu schreibenden Bytes
*/
void out_write ( size_t size, const void* buffer );

/*
**  Buffer für das Schreiben in das OUT-File anfordern
**  return : Zeiger auf den Buffer, bleibt bis zum nächsten Aufruf von
**           out_getbuffer(), out_write() oder out_filelength() gültig.
*/
void* out_getbuffer ( size_t size );

/*
**  Rückgabe nicht angeforderter aber nicht benötigter Bytes
**  size : Anzahl der nicht benötigten Bytes
*/
void out_ungetbuffer ( size_t size );

/*
**  Schreibt eine Byte in das OUT-File (über den out_buffer)
**  byte : das zu schreibende Byte
*/
void out_writebyte ( BYTE byte );

/*
**  lseek zu absoluter Position im OUT-File mit vorherigem Schreiben
**  des out_buffers
**  offset : Position im File
*/
void out_seek ( long offset );

/*
**  Bestimmen der aktuelle OUT Filegröße
**  return : aktuelle Länge des OUT-Files in Byte
*/
long out_filelength ( void );

/**** in anderem Modul **********************************************/

/*
**  Programmname
*/
extern const char *const progname;

/*
**  Programmfunktion
*/
extern const char *const progfunktion;

/*
**  Ergänzende Beschreibung der verfügbaren Schalter
**  oder NULL
*/
extern const char* const manual2;

/*
**  Default Extension des Output-Filenamens
*/
extern const char *const extension;

/*
**  Füllen des Buffers, wird in den einzelnen Modulen erledigt,
**  weil für das BMP-Format die Reichenfolge der Zeilen
**  umgekehrt werden muß.
*/
void img_fillbuffer ( void );

/*
**  Auswerten von Schaltern aus der Kommandozeile für das
**  spezielle Modul
**  sw     : der Schalter (ohne das Schaltersymbol '-')
**  return : War der Schalter gültig ?
*/
int schalter2 ( const char* sw );

/*
**  Durchführen der Transformation   IMG => ?
**  skala      : Zuordnungsart Grauwert => RGB-Wert
**  compressed : Soll das File komprimiert werden ?
*/
void trafo ( img_skalierung skala, int compressed );
