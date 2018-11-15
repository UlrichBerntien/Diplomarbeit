/*
**  Zortech C V2.1
**  .04.1994  Ulrich Berntien
**
**  Kern für Umwandlungsprogramme in das IMG-Grafikformak (dieses Grafik-
**  Format wird in Programmen wie IS31, IRIS am Institut für Experimental-
**  physik der Uni Düsseldorf benutzt.)
**  Der Kern enthält die Benutzerschnittstelle, die Schnittstelle zur
**  Ein- und Ausgabedatei und die Fehlerbehandlung.
**  Das Lesen und Interpretieren des Ursprungs-Grafikformats wird in Modulen
**  wie BMP2.C durchgeführt mithilfe von hier implementierten Funktionen.
**
**  16.04.1994  Entstanden aus img2x.c
**  07.05.1994  x_manual und x_schalter() aufgenommen
**  07.12.1994  Datum des IMG-Files wird auf das Datum des Input-Files gesetzt
*/

#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <io.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "x2img.h"

/********************************************************************/

/*
**  Kurzbeschreibung des Programms
*/
static const char manual [] =
  "%s     .04.1994     Ulrich Berntien\n"
  "Institut für Experimentalphysik der HHUD\n"
  "Umwandlungsprogramm für Grafiken in das instituts-\n"
  "interne IMG-Format (256 oder 512 Zeilen).\n"
  "\n"
  "Aufrufformat:\n"
  "  %s [schalter] inputfile [ imgfile ]\n"
  "\n"
  "Wird beim \"inputfile\" keine Extension angegeben, so wird\n"
  "die Extension \"%s\" benutzt.\n"
  "Wird beim \"imgfile\" keine Extension angegeben, so wird die\n"
  "Extension \".IMG\" benutzt.\n"
  "Wird kein \"imgfile\" angegeben, so wird ein File mit dem Namen\n"
  "des \"inputfile\" mit der Extension \".IMG\" angelegt.\n"
  "Beginnt ein Filename mit einem '-', so muß es zu '--' verdoppelt\n"
  "werden damit die Unterscheidung mit einem Schalter möglich ist.\n"
  "Existiert das \"imgfile\" bereits, so wird eine Frage nach dem\n"
  "Überschreiben des Files ausgegeben. Dieses Verhalten läßt sich\n"
  "mit den Schaltern j/n verändern.\n"
  "\n"
  "mögliche Schalter:\n"
  "  -v     immer ein IMG-File mit 512 Zeilen ausgeben.\n"
  "  -h     immer ein IMG-File mit 256 Zeilen ausgeben.\n"
  "  -x<n>  Startspalte des IMG-Files im Input-Bild.\n"
  "         Defaultwert 0\n"
  "  -y<n>  Startzeile des IMG-Files im Input-Bild.\n"
  "         Defaultwert 0\n"
  "  -b<n>  Grauwert für Pixel im IMG-File, die nicht durch das\n"
  "         Input-Bild definiert sind. Defaultwert 0\n"
  "  -j     ein existierendes IMG-File wird ohne Nachfrage\n"
  "         überschrieben.\n"
  "  -n     ein existierendes IMG-File wird nicht überschrieben.\n"
  "";

/*
**  Größe der Ein/Ausgabe-Buffer für die Files in Bytes
*/
static const buffersize = 48 * 1024;

/*
**  Funktionen zur Fehlerausgabe
**
\********************************************************************/

/*
**  Fehlermeldung mit Programmabbruch
**  form : zur Ausgabe über vfprintf()
*/
void err_abort ( const char* form, ... )
  {
  va_list args;
  va_start( args, form );
  vfprintf( stderr, form, args );
  fputs( "\nProgramm wird abgebrochen.\n", stderr );
  va_end( args );
  img_rette();
  exit( EXIT_FAILURE );
  }

/*
**  Programminterner Fehler führt zu einem Abbruch
**  fkt  : Name der Funktion, in der der Fehler gefunden wurde
**  form : zur Ausgabe über vfprintf()
*/
void err_intern ( const char* fkt, const char* form, ... )
  {
  va_list args;
  va_start( args, form );
  fprintf( stderr, "Interner Fehler in Funktion %s() aufgetreten.\n", fkt );
  vfprintf( stderr, form, args );
  fputs( "\nProgramm wird abgebrochen.\n", stderr );
  va_end( args );
  img_rette();
  exit( EXIT_FAILURE );
  }

/*
**  Ausgabe einer Warmeldung, ohne Programmabbruch
**  form : zur Ausgabe über vfprintf()
*/
void err_message ( const char* form, ... )
  {
  va_list args;
  va_start( args, form );
  fputs( "Warnung: ", stderr );
  vfprintf( stderr, form, args );
  fputc( '\n', stderr );
  va_end( args );
  }

/*
**  Ausgabe einer Fehlermeldung, wenn ein Fehler in der Kommandozeile
**  vom Benutzer gemacht wurde
**  form : zur Ausgabe über vfprintf()
*/
static void err_cmdline ( const char* form, ... )
  {
  va_list args;
  va_start( args, form );
  fputs( "Fehler in der Aufrufzeile des Programms:\n", stderr );
  vfprintf( stderr, form, args );
  fprintf( stderr, "\nInformationen über das Aufrufformat mit \"%s -?\".\n",
                   x_name );
  va_end( args );
  exit( EXIT_FAILURE );
  }

/*
**  Funktionen zur Speicherverwaltung mit Fehlerkontrolle
**
\********************************************************************/

/*
**  malloc mit Fehlerkontrolle
**  size   : Anzahl der benötigeten Bytes
**  return : Zeiger auf den angeforderten Speicherbereich
*/
void* ok_malloc ( size_t size )
  {
  void *const result = malloc( size );
  if( result == NULL )
    err_abort( "Hauptspeicher zu klein. (malloc(%u) gescheitert.)", size );
  return result;
  }

/*
**  Funktionen und Daten zum File-I/O mit Fehlerkontrolle
**
\********************************************************************/

/*
**  Alle File-IO können sich nur auf des Input-File oder das
**  Output-File (IMG-File) beziehen:
*/
typedef enum io_kanalTAG
  {
  _io_input, _io_output
  }
  io_kanal;

/*
**  Namen und DOS-Filehandles der beiden Files
*/
static const char* io_name [2];
static int io_handle [2];

/*
**  Ausgebe einer Fehlermeldung und Programmbeneden
**  kanal     : das File in dem der Fehler aufgetreten
**  operation : Name der Operation bei der der Fehler aufgetreten
*/
static void io_error ( io_kanal kanal, const char* operation )
  {
  static const char *const gname [2] = { "Eingabe", "Ausgabe" };
  const int code = errno;
  if( code != 0 )
    fprintf( stderr, "(Laufzeitbibliothek meldet Fehler: %s.)\n", strerror(code) );
  err_abort( "Beim %s der %s-Datei \"%s\" ist ein Fehler aufgetreten.",
              operation, gname[kanal], io_name[kanal] );
  }

/*
**  Öffnen eines Files
**  kanal : als was soll das File geöffnet werden ?
**  name  : Name des Files (mit Pfad und Extension), muß erhalten bleiben
*/
static void io_open ( io_kanal kanal, const char* name )
  {
  static const char *const operation[2] = { "Öffnen", "Anlegen" };
  int handle;
  errno = 0;
  io_name[kanal] = name;
  if( kanal == _io_input )
    handle = dos_open( name, O_RDONLY );
  else
    handle = dos_creat( name, 0 );
  if( handle < 0 ) io_error( kanal, operation[kanal] );
  io_handle[kanal] = handle;
  }

/*
**  Schließen eines Files
**  kanal : der zu schließende Kanal
*/
static void io_close ( io_kanal kanal )
  {
  errno = 0;
  if( close( io_handle[kanal] ) < 0 ) io_error( kanal, "Schließen" );
  }

/*
**  Lesen aus dem Input-File
**  buffer : dorthin die gelesenen Daten schreiben
**  size   : Anzahl der zu lesenden Bytes
*/
static void io_read ( void* buffer, size_t size )
  {
  int result;
  errno = 0;
  result = read( io_handle[_io_input], buffer, size );
  if( result != size ) io_error( _io_input, "Lesen aus" );
  }

/*
**  Lesen aus dem Input-File, wobei die angegeben Anzahl der Bytes
**  nicht erreicht werden braucht.
**  buffer : dorthin die gelesenen Daten schreiben
**  size   : Anzahl der maximal zu lesenden Bytes
**  return : Anzahl der gelesenen Bytes (immer >0)
*/
static unsigned io_readmax ( void* buffer, size_t size )
  {
  int result;
  errno = 0;
  result = read( io_handle[_io_input], buffer, size );
  if( result == -1 ) io_error( _io_input, "beschränktes Lesen aus" );
  return (unsigned) result;
  }

/*
**  Schreiben in das Output-File
**  buffer : aus diesem Buffer die Daten ins File schreiben
**  size   : Anzahl der zu schreibenden Bytes
*/
static void io_write ( void* buffer, size_t size )
  {
  int result;
  errno = 0;
  result = write( io_handle[_io_output], buffer, size );
  if( result != size ) io_error( _io_output, "Schreiben in" );
  }

/*
**  Positionieren des Read/Write-Pointers bezüglich Fileanfang
**  kanal    : in diesem File positionieren
**  position : auf diese Position setzen
*/
static void io_seek ( io_kanal kanal, long position )
  {
  long result;
  errno = 0;
  result = lseek( io_handle[kanal], position, SEEK_SET );
  if( result != position ) io_error( kanal, "Seek in" );
  }

/*
**  Größe einer Datei abfragen
**  (nur für die Input-File implementiert, weil es Probleme mit
**   der Laufzeitbibliothek/DOS gibt bei der Output-File)
**  kanal  : von dieser Datei die Größe erfragen
**  return : Anzahl der Bytes in der Datei
*/
static long io_filelength ( io_kanal kanal )
  {
  long result;
  if( kanal == _io_input )
    result = filelength( io_handle[_io_input] );
  else
    err_intern( "io_filelength", "Funktion nur für Eingabedatei erlaubt" );
  return result;
  }

/*
**  Funktionen und Daten zur Ausgabe in das IMG-File (Output-File)
**
**  Das IMG-File erscheint den Modulen, die das Input-File lesen,
**  als eine Folge von Grauwerten (jeder Grauwert 8Bit).
**  Durch Aufruf der Funktion img_start() wird die Ausgabe in das
**  IMG-File vorbereitet.
**  Druch die Funktion img_pixel() werden alle Grauwerte übergeben.
**  Das Modul hat dabei ggf. für die Umwandlung Farbwert in Grauwert
**  zu sorgen. Die Umwandlung der Größe des Bildes erfolgt erst hier.
**  Der Funktion img_pixel() werden die Grauwerte des Bilds in jeder
**  Zeile von links nach rechts übergeben. Die Reihenfolge der Zeilen
**  kann von oben nach unten oder umgekehrt erfolgen.
**  In jeder Zeile müssen immer alle Pixel übergeben werden. (Die
**  Anzahl wurde in img_start() übergeben.)
**
\********************************************************************/

/*
**  Anzahl der Zeilen in einem IMG-Vollbild und IMG-Halbbild
*/
static const int img_vollbild = 512;
static const int img_halbbild = 256;

/*
**  Anzahl der Spalten in einem IMG-File (Voll- und Halbbild gleich)
*/
static const unsigned img_spalten = 512;

/*
**  Standard Extensions für Namen von IMG-Files
*/
static const char img_extension [] = ".IMG";

/*
**  Struktur zu Aufnahme der Daten für das IMG-File (Output-File)
*/
typedef struct img_structTAG
  {
  BYTE backcolor;        /* Farbe des Hintergrunds                    */
  int startx;            /* Ab dieser Spalte beginnt das IMG-Bild     */
  int starty;            /* Ab dieser Zeile beginnt das IMG-Bild      */
  int sollzeilen;        /* vorgegebene Zeilenanzahl für das IMG-Bild */
                         /* 256/512 oder 0 -> in img_start() wählen   */
  /*--------------------*/
  int isInit;            /* wurde img_start aufgerufen                */
  int delta;             /* je nach Reihenfolge der Zeilen +1 oder -1 */
  int endx;              /* bis zu dieser Spalte reicht das IMG-File  */
  int endy;              /* bis zu dieser Zeile reicht das IMG-File   */
  int spalten;           /* Anzahl der Spalten des Ursprungsbilds     */
  int zeilen;            /* Anzahl der Zeilen des Ursprungsbilds      */
  int x;                 /* Aktuelle Spalte des Ursprungsbilds        */
  int y;                 /* Aktuelle Zeile des Ursprungsbilds         */
  int imgrest;           /* noch zu schreibende Zeilen im IMG-File    */
  int inzeile;           /* werden gerade Zeilen innerhalb des IMG-
                            Bilds geschrieben ?                       */
  int enter;             /* Zähler wieviele img_*() Funktionen aktiv  */
  /*--------------------*/
  BYTE* buffer;          /* Buffer für das IMG-File                   */
  BYTE* zeile;           /* aktuelle Zeile im Buffer                  */
  unsigned size;         /* Anzahl verfügbarer Zeilen im Buffer       */
  int now;               /* Nummer der aktuellen Zeile im Buffer      */
  int first;             /* Nummer der ersten Zeile im Buffer         */
  }
  img_struct;

/*
**  Daten zur Verwaltung des IMG-Files
*/
static img_struct img_data = { 0, 0, 0, 0, 0  };

/*
**  Initialisieren des Buffers für das IMG-File
*/
static void img_bufferinit ( void )
  {
  const size_t prozeile = img_spalten * sizeof (BYTE);
  img_data.size = buffersize / prozeile;
  if( img_data.size > img_data.imgrest ) img_data.size = img_data.imgrest;
  img_data.buffer = ok_malloc( prozeile * img_data.size );
  if( img_data.delta < 0 )
    {
    img_data.first = img_data.imgrest - img_data.size;
    img_data.now = img_data.size - 1;
    }
  else
    {
    img_data.first = 0;
    img_data.now = 0;
    }
  img_data.zeile = img_data.buffer + img_data.now * img_spalten;
  }

/*
**  Schreiben des Buffers für das IMG-File
*/
static void img_bufferflush ( void )
  {
  if( img_data.size > 0 )
    {
    io_seek( _io_output, img_spalten * (long) img_data.first );
    io_write( img_data.buffer, img_data.size * img_spalten );
    if( img_data.delta > 0 ) img_data.first += img_data.size;
    if( img_data.size > img_data.imgrest ) img_data.size = img_data.imgrest;
    if( img_data.delta < 0 ) img_data.first -= img_data.size;
    img_data.now = img_data.delta < 0 ? img_data.size - 1 : 0;
    img_data.zeile = img_data.buffer + img_data.now * img_spalten;
    }
  }

/*
**  Umschalten auf die nächste Zeile im Buffer
**  ggf. mit Ausleeren des Buffers
*/
static void img_buffernext ( void )
  {
  img_data.now += img_data.delta;
  img_data.zeile = img_data.buffer + img_data.now * img_spalten;
  --img_data.imgrest;
  if( img_data.now < 0 || img_data.now >= img_data.size ) img_bufferflush();
  if( img_data.imgrest < 0 )
    err_intern( "img_buffernext", "Zuviele IMG-Filezeilen geschrieben." );
  }

/*
**  Beginn der Ausgabe in das IMG-File, Informationen über das Bild geben
**  breite  : Anzahl der Spalten im Input-Bild
**  hoehe   : Anzahl der Zeilen im Input-Bild
**  vonoben : Kommt die oberste Zeile im Bild zuerst ?
*/
void img_start ( int breite, int hoehe, int vonoben )
  {
  img_data.isInit = 1;
  img_data.delta = vonoben ? +1 : -1;
  img_data.endx = img_data.startx + img_spalten;
  if( img_data.sollzeilen == 0 )
    {
    if( img_data.starty + img_halbbild < hoehe )
      img_data.imgrest = img_vollbild;
    else
      img_data.imgrest = img_halbbild;
    }
  else
    img_data.imgrest = img_data.sollzeilen;
  img_data.endy = img_data.starty + img_data.imgrest;
  img_data.spalten = breite;
  img_data.zeilen = hoehe;
  img_data.x = 0;
  img_data.y = vonoben ? 0 : img_data.zeilen - 1;
  img_data.enter = 0;
  img_data.inzeile = img_data.y >= img_data.starty && img_data.y < img_data.endy;
  img_bufferinit();
  if( img_data.inzeile )
    {
    int leerzeilen;
    if( vonoben )
      leerzeilen = img_data.y - img_data.starty;
    else
      leerzeilen = img_data.endy - img_data.y - 1;
    while( leerzeilen > 0 )
      {
      memset( img_data.zeile, img_data.backcolor, img_spalten );
      img_buffernext();
      --leerzeilen;
      }
    }
  printf( "Schreibe IMG-File mit %d Zeilen\n", img_data.imgrest );
  }

/*
**  IMG-File ggf. mit Leerzeilen auffüllen,
**  schreiben des Bufferinhalts ins File,
**  schließen des Files
*/
static void img_close ( void )
  {
  ++img_data.enter;
  while( img_data.imgrest > 0 )
    {
    memset( img_data.zeile, img_data.backcolor, img_spalten );
    img_buffernext();
    }
  img_bufferflush();
  io_close( _io_output );
  --img_data.enter;
  }

/*
**  Setzt Datum/Uhrzeit des IMG-Files (Outputfiles) auf Datum/Uhrzeit
**  des Input-Files.
*/
static void img_setdate ( void )
  {
  unsigned date;
  unsigned time;
  if( dos_getftime( io_handle[_io_input], &date, &time ) == 0 )
    dos_setftime( io_handle[_io_output], date, time );
  }

/*
**  Ausgeben eines Pixels in das IMG-File
**  wert : Grauwert (0..255)
*/
void img_pixel ( BYTE wert )
  {
  const int inspalte = img_data.x >= img_data.startx && img_data.x < img_data.endx;
  ++img_data.enter;
  if( img_data.inzeile && inspalte )
    img_data.zeile[img_data.x-img_data.startx] = wert;
  if( ++img_data.x >= img_data.spalten )
    {
    if( img_data.inzeile && img_data.x < img_data.endx )
      memset( img_data.zeile + ( img_data.x - img_data.startx ),
              img_data.backcolor,
              img_data.endx - img_data.x );
    img_data.x = 0;
    img_data.y += img_data.delta;
    if( img_data.inzeile ) img_buffernext();
    img_data.inzeile = img_data.y >= img_data.starty && img_data.y < img_data.endy;
    }
  --img_data.enter;
  }

/*
**  Versucht nach einem aufgetreten Fehler das IMG-File so
**  vollständig wie möglich zu Retten
*/
void img_rette ( void )
  {
  if( img_data.isInit && img_data.inzeile && ! img_data.enter )
    {
    ++img_data.enter;
    if( img_data.x >= img_data.startx && img_data.x < img_data.endx )
      memset( img_data.zeile + ( img_data.x - img_data.startx ),
              img_data.backcolor,
              img_data.endx - img_data.x );
    img_close();
    --img_data.enter;
    }
  }

/*
**  Funktionen und Daten zum Lesen des Input-Files
**
\********************************************************************/

/*
**  Struktur zur Aufnahme der Daten für den Input-Buffer
*/
typedef struct in_structTAG
  {
  BYTE* buffer;         /* auf den Buffer                              */
  unsigned index;       /* Index des nächsten zu lesenden Bytes        */
  unsigned avail;       /* Anzahl der noch vorhandenen Bytes im Buffer */
  unsigned size;        /* Größe des angelegten Buffers                */
  }
  in_struct;

/*
**  Daten zur Verwaltung des Input-Buffers
*/
static in_struct in_data = { NULL, 0, 0, 0 };

/*
**  Bestimmen der Größe des Inputfiles
**  return : Größe des Files in Bytes
*/
long in_filelength ( void )
  {
  static long length = -1;
  if( length < 0L ) length = io_filelength( _io_input );
  return length;
  }

/*
**  Lesen aus dem Inputfile ohne Bufferbenutzung
**  position : ab dieser Position (relativ zum Fileanfang) lesen
**  buffer   : in diesem Buffer die gelesen Daten schreiben
**  size     : Anzahl der zu lesenden Bytes
*/
void in_readdirect ( long position, void* buffer, size_t size )
  {
  io_seek( _io_input, position );
  io_read( buffer, size );
  }

/*
**  Auffüllen des Buffers ab der aktuellen Fileposition
*/
static void in_fillbuffer ( void )
  {
  unsigned read;
  if( in_data.avail > 0 )
    memmove( in_data.buffer, in_data.buffer + in_data.index, in_data.avail );
  in_data.index = 0;
  read = io_readmax( in_data.buffer, in_data.size - in_data.avail );
  in_data.avail += read;
  }

/*
**  Buffer neu füllen
**  position : ab dieser Psition (relativ zum Fileanfang) lesen
*/
void in_newbuffer ( long position )
  {
  io_seek( _io_input, position );
  if( in_data.buffer == NULL )
    {
    in_data.buffer = ok_malloc( buffersize );
    in_data.size = buffersize;
    in_data.index = 0;
    }
  in_data.avail = 0;
  in_fillbuffer();
  }

/*
**  Ein Byte aus dem Buffer lesen
**  return : das Byte aus dem Buffer
*/
BYTE in_readbyte ( void )
  {
  if( in_data.avail == 0 ) in_fillbuffer();
  --in_data.avail;
  return in_data.buffer[in_data.index++];
  }

/*
**  Bytes aus dem Buffer lesen
**  buffer : dort hin die gelesen Daten schreiben
**  size   : Anzahl der zu lesenden Bytes
*/
void in_read ( void* buffer, size_t size )
  {
  while( size > 0 )
    {
    unsigned stueck;
    if( in_data.avail == 0 ) in_fillbuffer();
    stueck = size < in_data.avail ? size : in_data.avail;
    memcpy( buffer, in_data.buffer + in_data.index, stueck );
    in_data.avail -= stueck;
    in_data.index += stueck;
    size -= stueck;
    }
  }

/*
**  Funktionen und Daten der Benutzerschnittstelle
**
\********************************************************************/

/*
**  Soll automatisch überschrieben werden (-1) oder nicht (+1)
**  Standard-Wert : 0 nachfragen
*/
static int fileschutz = 0;

/*
**  Überprüft in Abhängigkeit von der globalen Variablen 'fileschutz'
**  ob die Datei erzeugt werden darf
**  filename : Name der zu erzeugenden Datei
*/
static void filecontrol ( const char* filename )
  {
  if( access( filename, F_OK ) == 0 )
    switch( fileschutz )
      {
      case +1 : fprintf( stderr, "IMG-File \"%s\" existiert.\n", filename );
                exit( 2 );
      case -1 : break;
      case  0 : {
                char in;
                printf( "Soll File \"%s\" überschrieben werden (J/N) ?\n", filename );
                do{
                  in = getch();
                  in = tolower( in );
                  }
                  while( in != 'n' && in != 'j' );
                if( in == 'n' ) exit( 2 );
                }
      }
  }

/*
**  Überprüft, ob ein Dateiname mit Extension angegeben ist
**  name   : der Dateiname
**  return : Eine Extension angegeben ?
*/
static int extensionexist ( const char* name )
  {
  int dot = 0;
  int n = strlen( name ) - 1;
  while( n >= 0 && name[n] != '/' && name[n] != '\\' && name[n] != ':' )
    if( name[n--] == '.' ) dot = 1;
  return dot;
  }

/*
**  Ein Dateinamen wird mit einer gegebenen Extension versehen, eine
**  ggf. vorhandene Extension wird überschrieben
**  name      : ursprünglicher Dateinamen
**  extension : der Dateinamen bekommt diese Erweiterung
**              (der '.' ist in extension enthalten)
**  return    : erzeugter Dateinamen (free nicht vergessen)
*/
static char* extensionadd ( const char* name, const char* extension )
  {
  const int namelen = strlen( name );
  char *const result = ok_malloc( namelen + 1 + strlen(extension) );
  int dot = namelen;
  int i;
  strcpy( result, name );
  i = namelen;
  while( i >= 0 && name[i] != '/' && name[i] != '\\' && name[i] != ':' )
    {
    if( name[i] == '.' )
      {
      dot = i;
      break;
      }
    --i;
    }
  strcpy( result + dot, extension );
  return result;
  }

/*
**  Decodiert eine positive Dezimalzahl aus einem String
**  zahl   : String der die Zahl enthält
**  max    : größter erlaubter Wert
**  sw     : Name des Schalters, bei dem diese Zahl angegeben ist
**  return : Der Wert der Zahl
*/
static int readplus ( const char* zahl, int max, char sw )
  {
  int result;
  int i = 0;
  while( zahl[i] != '\0' && isdigit( zahl[i] ) ) ++i;
  if( zahl[i] != '\0' )
    err_cmdline( "Keine gültige Zahl im Schalter \"%c\" angegeben.", sw );
  result = atoi( zahl );
  if( result < 0 || result > max )
    err_cmdline( "Die Zahl im Schalter \"%c\" ist größer als erlaubt(%d).", sw, max );
  return result;
  }

/*
**  Auswerten eines Schalters von der Kommandozeile
**  sw : der Schalter ohne das Schaltersymbol
*/
static void schalter ( const char* sw )
  {
  static const char format [] = "Unbekannter Schalter \"%s\".";
  switch( sw[0] )
    {
    case 'v' : img_data.sollzeilen = img_vollbild;
               break;
    case 'h' : img_data.sollzeilen = img_halbbild;
               break;
    case 'x' : img_data.startx = readplus( sw+1, INT_MAX, sw[0] );
               break;
    case 'y' : img_data.starty = readplus( sw+1, INT_MAX, sw[0] );
               break;
    case 'b' : img_data.backcolor = readplus( sw+1, 255, sw[0] );
               break;
    case 'n' :
    case 'j' : if( sw[1] != '\0' ) err_cmdline( format, sw );
               fileschutz = sw[0] == 'j' ? -1 : +1;
               break;
    case '?' : printf( manual, x_name, x_name, x_extension );
               if( x_manual != NULL ) puts( x_manual );
               exit( EXIT_SUCCESS );
    default  : if( ! x_schalter( sw ) ) err_cmdline( format, sw );
    }
  }

/*
**  M A I N
*/
int main ( int argc, char* argv [] )
  {
  const char* inputfile = NULL;
  const char* outputfile = NULL;
  if( argc < 2 )
    err_cmdline( "Keine Argumente angegeben." );
  while( ++argv, --argc > 0 )
    if( (*argv)[0] == '-' && (*argv)[1] != '-' )
      schalter( *argv + 1 );
    else
      {
      const int offset = (*argv)[0] == '-' ? 1 : 0;
      if( inputfile == NULL )
        inputfile = *argv + offset;
      else if( outputfile == NULL )
        outputfile = *argv + offset;
      else
        err_cmdline( "Nur zwei Dateinamen möglich, aber einen dritten gefunden" );
      }
  if( inputfile == NULL )
    err_cmdline( "Name des Eingabefiles ist nicht angegeben." );
  if( outputfile == NULL )
    outputfile = extensionadd( inputfile, img_extension );
  else
    if( ! extensionexist( outputfile ) )
      outputfile = extensionadd( outputfile, img_extension );
  if( ! extensionexist( inputfile ) )
    inputfile = extensionadd( inputfile, x_extension );
  filecontrol( outputfile );
  io_open( _io_input, inputfile );
  io_open( _io_output, outputfile );
  x_trafo();
  img_setdate();
  img_close();
  io_close( _io_input );
  return EXIT_SUCCESS;
  }
