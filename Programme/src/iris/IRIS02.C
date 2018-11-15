/*
**  C    ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .10.1992
**
**  Halbbilder/Vollbilder in Files speichern und laden
**
**  02.10.1992  Aufteilung in 'iris01.c' und 'iris02.c'
**  09.02.1993  Funktionen 'getpath' und 'dateiname' von iris02 nach iris00
**  11.06.1993  Pfadname beim Speichern der Bilder wird nur einmal abgefragt
**  23.08.1993  um Speichern/Laden von Vollbildern erweitert
**  25.07.1993  um Laden von Vollbildern im Institutseigenem Format erweitert
**  19.12.1994  um Speichern von abgesplittetes Halbbild erweitert
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <dos.h>
#include "iris.h"
#include "isdefs.h"

/********************************************************************/

#ifdef __ZTC__
#  define OPENP
#else
#  define OPENP (char*)       /* in ZTC open 1.Parameter const char* */
#endif

#define buffersize (16*1024)                  /* Bytes für Filebuffer */
#define filesize (512L*256L+36L)  /* Bytes in File mit einem Halbbild */

/*
**  Standart-Filename-Extension für Halbbilder
*/
TEXT ext [] = "IMG";

/********************************************************************/

/*
**  Legt Buffer für Filezugriff an
**  size   : Zeiger auf Angabe der gewünschten Buffergröße in Bytes
**           bei Rückgabe wird dort die angelegt Buffergröße übergeben
**  return : der angelegte Buffer oder NULL im Fehlerfall
*/
static char* LOCAL newbuffer ( unsigned* size )
  {
  unsigned bsize = buffersize;
  char* buffer = malloc ( bsize );
  while( buffer == NULL && bsize > bpz )
    {
    bsize /= 2;
    buffer = malloc( bsize );
    }
  if( buffer == NULL ) errmalloc();
  *size = bsize;
  return buffer;
  }

/*
**  Informiert Benutzer über Speicherplatz auf dem Laufwerk für Halbbilder
**  path : Pfadname ggf. mit Laufwerksangabe
*/
static void LOCAL checkfree ( const char* path )
  {
  const int drive = path[1] == ':' ? toupper(path[0]) - 'A' +1: 0;
  const long free = diskfree( drive );
  const long bilder = free / filesize;
  if( bilder < 1 )
    {
    if( drive )
      printf( "Auf Laufwerk '%c:'", drive - 1 + 'A' );
    else
      fputs( "Auf aktuellem Laufwerk", stdout );
    printf( " sind noch %ld KBytes frei.\n"
            "Ein Halbbild benötigt %ld KBytes.\n" ,
            free / 1024L,  filesize / 1024L );
    getreturn();
    }
  else
    if( bilder > 1 )
      printf( "Speicher für ca. %ld Bilder, ", bilder );
    else
      fputs( "Speicher für ca. ein Bild, ", stdout );
  }

/*
**  Erzeugt neue Datei, gibt ggf. Fehlermeldung aus
**  fname  : vollständiger Filename
**  return : Filehandle oder -1 im Fehlerfall
*/
static int LOCAL datei ( const char* fname )
  {
  int handle;
  int omode = S_IWRITE | S_IREAD;
  int oflag = O_BINARY | O_CREAT | O_EXCL | O_WRONLY;
  handle = open( OPENP fname, oflag, omode );
  if( handle < 0 && errno == EEXIST )
    {
    printf( "Die Datei '%s' besteht bereits.\nÜberschreiben", fname );
    clearkbd();
    if( janein() )
      handle = open( OPENP fname, oflag & ~O_EXCL, omode );
    }
  if( handle < 0 )
    printf( "Die Datei '%s' kann nicht erstellt werden.\n", fname );
  return handle;
  }

/*
**  Nachspann für ein Halbbild in die Datei 'handle' geben
**  Der Nachspann ist 36 Bytes lang,
**  in ihm ist Datum, Zeit, Framebuffernummer, Framegrabbernummer gespeichert.
**    Bytes       |        Inhalt
**  --------------+-------------------------------------------------------
**   00,01        |   Tag    codiert "01".."31"
**   02,03        |   Monat  codiert "01".."12"
**   04,05        |   Jahr   codiert "00".."99"
**   10,11        |   Stunde codiert "00".."23"
**   12,13        |   Minute codiert "00".."59"
**   16           |   Zeichen 'e' falls keine Multiplexerkarte DT2859 sonst 'm'
**   17           |   F_GRAB_ZAHL, hier gleich 3
**   18           |   Frame Buffer Nummer,  hier einfach auf 0 gesetzt
**   19           |   Frame Grabber Nummer, hier einfach auf 1 gesetzt
**   33           |   auf Wert 0x02 gesetzt
**   35           |   auf Wert 0x01 gesetzt
**  alle anderen  |   auf Wert 0 gesetzt
*/
static void LOCAL nachspann ( int handle )
  {
  char buffer [ 36 ];
  struct tm *local;
  time_t now;
  memset( buffer, 0, sizeof buffer );
  time( &now );
  local = localtime( &now );
  sprintf( buffer, "%2d%2d%2d",
           local->tm_mday, local->tm_mon+1, local->tm_year % 100 );
  sprintf( buffer+10, "%2d%2d", local->tm_hour, local->tm_min );
  buffer[16] = 'm';
  buffer[17] = 0x03;
  buffer[19] = 0x01;
  buffer[33] = 0x02;
  buffer[35] = 0x01;
  write( handle, buffer, sizeof buffer );
  }

/*
**  Speichert zpb/2 Zeilen des Bildes ab (ein Halbbild)
**  frb    : Framebuffer Nummer
**  zeile  : Nummer der ersten Zeile die gespeichert wird (0..255)
**  fname  : der vollständige Filenamen
**  return : Ist ein Fehler aufgetreten ?
*/
static int LOCAL savehalb ( int frb, int zeile, const char* fname )
  {
  unsigned size;
  char* buffer = newbuffer( &size );
  const int zpbuffer = size / bpz;
  int ok;
  int handle;
  int letzte;
  ok = buffer != NULL;
  if( ok )
    {
    checkfree( fname );
    ok = ( handle = datei( fname ) ) > -1;
    }
  if( ok )
    printf( "schreibe Datei '%s'\n", fname );
  letzte = zeile + zpb / 2;
  while( ok && zeile < letzte )
    {
    isb_get_pixel( frb, zeile, size/2, buffer );
    if( size != write( handle, buffer, size ) )
      {
      puts( "Fehler beim Schreiben in Datei, Disk voll ?" );
      ok = 0;
      }
    zeile += zpbuffer;
    }
  free( buffer );
  if( ok )
    nachspann( handle );
  if( handle > 0 )
    {
    close( handle );
    if( ! ok ) unlink( fname );
    }
  return ! ok;
  }

/*
**  Speichert jede zweite Zeilen des Bildes ab (ein abgesplittetes Halbbild)
**  frb    : Framebuffer Nummer
**  zeile  : Nummer der ersten Zeile die gespeichert wird (0 oder 1)
**  fname  : der vollständige Filenamen
**  return : Ist ein Fehler aufgetreten ?
*/
static int LOCAL saveSplitter ( int frb, int zeile, const char* fname )
  {
  int ok;
  int handle;
  if( zeile > 1 || zeile < 0 ) zeile = 0;
  checkfree( fname );
  ok = ( handle = datei( fname ) ) > -1;
  if( ok )
    printf( "schreibe Datei '%s'\n", fname );
  while( ok && zeile < zpb )
    {
    char buffer [bpz];
    isb_get_pixel( frb, zeile, wpz, buffer );
    if( bpz != write( handle, buffer, bpz ) )
      {
      puts( "Fehler beim Schreiben in Datei, Disk voll ?" );
      ok = 0;
      }
    zeile += 2;
    }
  if( ok )
    nachspann( handle );
  if( handle > 0 )
    {
    close( handle );
    if( ! ok ) unlink( fname );
    }
  return ! ok;
  }

/*
**  Bild aus Framebuffer auf Ausgang zum Monitor legen
**  frb : Framebuffer Nummer
*/
static void LOCAL displayon ( int frb )
  {
  is_select_output_frame( frb );
  is_display( 1 );
  }

/********************************************************************/

/*
**  Pfadname in dem die Halbbilder gespeichert werden
*/
static char bildpath [ pathlen ];
/*
**  Enthält bildpath einen Pfad ?
*/
static int bildpathok = 0;

/*
**  Pfad zum Speichern der Halbbilder kann geändert werden
*/
void setbildpfad ( void )
  {
  puts( "Der Pfad in dem die Halbbilder gespeichert werden\n" );
  getpath( bildpath, pathlen );
  bildpathok = 1;
  }

/*
**  Frage nach Abbruch bei Fehler während des Speicherns
**  return : Soll abgebrochen werden ?
*/
static int LOCAL abbruch ( void )
  {
  fputs( "Wiederholen (mit anderem Namen)", stdout );
  clearkbd();
  return ! janein();
  }

/*
**  Neuer Filename mit Pfad zum Speichern abfragen
**  buffer : dort wird der neue Filename zusammengestellt
**  alt    : der alte Name
**  return : auf letztes Zeichen des neuen Namens
*/
static char* LOCAL neuername ( char* buffer, const char* alt )
  {
  char name [ namelen ];
  puts( "Halbbild unter Namen speichern:" );
  if( alt != NULL )
    strcpy( name, alt );
  else
    name[0] = '\0';
  getfname( name, namelen );
  strcpy( buffer, bildpath );
  getpath( buffer, pathlen );
  return dateiname ( buffer, name, ext );
  }

/*
**  Baut Namen zum Speichern der Halbbilder zusammen
**  ggf. den Pfad noch vom Benutzer erfragen
**  buffer : dort wird der Filename zusammengebaut
**  name   : der Filename (ohne Pfad und ohne Extension)
**  return : auf letztes Zeichen des Namens
*/
static char* LOCAL bauename ( char* buffer, const char* name )
  {
  if( ! bildpathok )
    {
    getpath( bildpath, pathlen );
    bildpathok = 1;
    }
  strcpy( buffer, bildpath );
  return dateiname( buffer, name, ext );
  }

/*
**  Benutzerdialog zum Speichern der Halbbilder
**  frb  : Framebuffer Nummer
**  name : Filename, ohne Pfad und ohne Extension
**         bei NULL wird der Name erfragt
*/
void savedialog ( int frb, const char* name )
  {
  char was;
  displayon( frb );
  puts( "Halbbilder speichern. Welche Bilder ?\n"
        "\t- - kein Bild\n"
        "\to - oberes Halbbild\n"
        "\tu - unteres Halbbild\n"
        "\tb - beide Halbbilder"  );
  was = getcset( "-oub" );
  if( was != '-' )
    {
    int fehler = name == NULL;
    do{
      char pathname [pathlen+namelen+extlen];
      char* last;
      if( fehler )
        last = neuername( pathname, name );
      else
        last = bauename( pathname, name );
      fehler = 0;
      if( was == 'b' ) *last = '1';
      if( was == 'b' || was == 'o' )
        fehler = savehalb( frb, 0, pathname );
      if( was == 'b' ) *last = '2';
      if( was == 'b' || was == 'u' )
        fehler = fehler || savehalb( frb, zpb/2, pathname );
      }
      while( fehler && ! abbruch() );
    }
  }

/*
**  Benutzerdialog zum Speichern eines abgesplitteten Halbbilds
**  frb  : Framebuffer Nummer
**  name : Filename, ohne Pfad und ohne Extension
**         bei NULL wird der Name erfragt
*/
void saveSplitterDialog ( int frb, const char* name )
  {
  int fehler = name == NULL;
  displayon( frb );
  do{
    char pathname [pathlen+namelen+extlen];
    if( fehler )
      neuername( pathname, name );
    else
      bauename( pathname, name );
    fehler = saveSplitter( frb, 0, pathname );
    }
    while( fehler && ! abbruch() );
  }

/********************************************************************/

/*
**  Öffnet Datei nur zum Lesen
**  name   : der vollständige Filename
**  return : Filehandle, oder < 0 im Fehlerfall
*/
static int LOCAL openfile ( const char* name )
  {
  int handle = open( OPENP name, O_RDONLY | O_BINARY );
  if( handle < 0 )
    printf( "Datei '%s' kann nicht geöffnet werden.\n", name );
  else
    printf( "Lese aus Datei '%s'\n", name );
  return handle;
  }

/*
**  Lesen aus einer Datei, gibt ggf. Fehlermeldung aus
**  handle : Filehandle (readonly)
**  buffer : dorthin die Daten schreiben
**  size   : Anzahl der zu lesenden Bytes
*/
static int LOCAL readfile ( int handle, void* buffer, size_t size )
  {
  const int ok = read( handle, buffer, size ) == size;
  if( ! ok )
    puts( "Fehler beim Lesen aus der Datei." );
  return ok;
  }

/*
**  Laden eines Voll- oder Halbbildes
**  das Halbbild wird beim Lesen sofort expandiert
**  frb      : Framebuffer Nummer
**  name     : vollständiger Filename
**  halbbild : Ist das Bild ein Halbbild ?
*/
static void LOCAL load ( int frb, const char* name, int halbbild )
  {
  int ok;
  unsigned size;
  char* buffer = newbuffer( &size );
  if( buffer != NULL )
    {
    int zeile = 0;
    int handle = openfile( name );
    ok = handle > -1;
    while ( ok > 0 && zeile < zpb )
      if( ( ok = readfile( handle, buffer, size ) ) > 0 )
        {
        unsigned offset;
        for( offset = 0; offset < size; offset += bpz )
          {
          isb_put_pixel( frb, zeile++, wpz, buffer + offset );
          if( halbbild ) isb_put_pixel( frb, zeile++, wpz, buffer + offset );
          }
        }
    close( handle );
    free( buffer );
    }
  if( buffer == NULL || ! ok ) getreturn();
  }

/*
**  Name für das Speichern/Laden eines Bilds erfragen
**  return : Zeiger auf den Name (mit Pfad und Extension)
**           oder NULL, wenn kein Name eingegben wurde
*/
static const char* getbildname ( void )
  {
  static char name [namelen];
  static char path [pathlen+namelen+extlen];
  char ext [extlen];
  getfname( name, namelen );
  if( name[0] != '\0' )
    {
    strcpy( path, bildpath );
    getpath( path, pathlen );
    puts( "Namenserweiterung" );
    strcpy( ext, "IMG" );
    getfname( ext, extlen );
    dateiname( path, name, ext );
    }
  return name[0] != '\0' ? path : NULL;
  }

/*
**  Dialog um ein Halbbild aus einem File zu laden
**  frb : Framebuffer Nummer
*/
void loaddialog ( int frb )
  {
  const char* name;
  displayon( frb );
  name = getbildname();
  if( name != NULL ) load( frb, name, 1 );
  }

/*
**  Dialog um ein Vollbild (nicht DT-IRIS-Format) aus einem File zu laden
**  frb : Framebuffer Nummer
*/
void loadvollNONIRIS ( int frb )
  {
  const char* name;
  displayon( frb );
  name = getbildname();
  if( name != NULL ) load( frb, name, 0 );
  }

/*
**  Vollbild (DT-IRIS-FORMAT) aus Datei laden
**  frb : Framebuffer Nummer
*/
void loadvoll ( int frb )
  {
  const char* name;
  displayon( frb );
  name = getbildname();
  if( name != NULL )
    if( is_restore( frb, 0,0, name ) ) getreturn();
  }

/*
**  Vollbild (DT-IRIS-FORMAT) in Datei speichern
**  frb : Framebuffer Nummer
*/
void savevoll ( int frb )
  {
  const char* name;
  displayon( frb );
  name = getbildname();
  if( name != NULL && access( name, 0 ) == 0 )
    {
    printf( "Datei '%s' überschreiben", name );
    if( ! janein() ) name = NULL;
    }
  if( name != NULL )
    if( is_save( frb, 0,1,8, name ) ) getreturn();
  }
