/*
**  Headerfile für das Modul X2IMG.C und alle Module
**  aus denen konvertiert wird BMP2.C, PCX2.C, GIF2.C und TIFF2.C
*/

/*
**  Elementare Datentypen
*/
typedef unsigned char  BYTE;             /*  8 Bit ohne Vorzeichen */
typedef unsigned short WORD;             /* 16 Bit ohne Vorzeichen */
typedef unsigned long  DWORD;            /* 32 Bit ohne Vorzeichen */
typedef long           LONG;             /* 32 Bit mit Vorzeichen  */

/*
**  Funktionen aus x2img.c für die Module zur Umwandlung x -> IMG
**
\********************************************************************/

/*
**  Fehlermeldung mit Programmabbruch
**  form : zur Ausgabe über vfprintf()
*/
void err_abort ( const char* form, ... );

/*
**  Programminterner Fehler führt zu einem Abbruch
**  fkt  : Name der Funktion, in der der Fehler gefunden wurde
**  form : zur Ausgabe über vfprintf()
*/
void err_intern ( const char* fkt, const char* form, ... );

/*
**  Ausgabe einer Warmeldung, ohne Programmabbruch
**  form : zur Ausgabe über vfprintf()
*/
void err_message ( const char* form, ... );

/*
**  malloc mit Fehlerkontrolle
**  size   : Anzahl der benötigeten Bytes
**  return : Zeiger auf den angeforderten Speicherbereich
*/
void* ok_malloc ( size_t size );

/*
**  Beginn der Ausgabe in das IMG-File, Informationen
**  über das Bild geben
**  breite  : Anzahl der Spalten im Input-Bild
**  hoehe   : Anzahl der Zeilen im Input-Bild
**  vonoben : Kommt die oberste Zeile im Bild zuerst ?
*/
void img_start ( int breite, int hoehe, int vonoben );

/*
**  Ausgeben eines Pixels in das IMG-File
**  wert : Grauwert (0..255)
*/
void img_pixel ( BYTE wert );

/*
**  Versucht nach einem Fehler das IMG-File so
**  vollständig wie möglich zu Retten
*/
void img_rette ( void );

/*
**  Bestimmen der Größe des Inputfiles
**  return : Größe des Files in Bytes
*/
long in_filelength ( void );

/*
**  Lesen aus dem Inputfile ohne Bufferbenutzung
**  position : ab dieser Position (relativ zum Fileanfang) lesen
**  buffer   : in diesem Buffer die gelesen Daten schreiben
**  size     : Anzahl der zu lesenden Bytes
*/
void in_readdirect ( long position, void* buffer, size_t size );

/*
**  Buffer neu füllen
**  position : ab dieser Psition (relativ zum Fileanfang) lesen
*/
void in_newbuffer ( long position );

/*
**  Ein Byte aus dem Buffer lesen
**  return : das Byte aus dem Buffer
*/
BYTE in_readbyte ( void );

/*
**  Bytes aus dem Buffer lesen
**  buffer : dort hin die gelesen Daten schreiben
**  size   : Anzahl der zu lesenden Bytes
*/
void in_read ( void* buffer, size_t size );

/*
**  Funktionen und Daten, die von den Modulen zur Umwandlung x -> IMG
**  zur Verfügung gesetellt werden müssen
**
\********************************************************************/

/*
**  Name des Programms
*/
extern const char *const x_name;

/*
**  Standard Extension der Files aus denen gelesen wird
*/
extern const char *const x_extension;

/*
**  Ergänzung der Programm-Kurzbeschreibung im X2IMG.C
**  oder NULL
*/
extern const char *const x_manual;

/*
**  Auswertung von speziellen Schaltern für das Modul
**  sw     : der Schalter, ohne das einführende Schaltersymbol
**  return : Konnte der Schalter ausgewertet werden ?
*/
int x_schalter ( const char* sw );

/*
**  Lesen des Input-Files über die in_*() Funktionen und schreiben
**  des IMG-Files über die img_*() Funktionen
*/
void x_trafo ( void );
