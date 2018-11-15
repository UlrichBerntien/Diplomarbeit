/*
**  Zortech C V2.1
**  .02.1994  Ulrich Berntien
**
**  Umwandlung von Bilddateien vom IMG-Format ins BMP, PCX, GIF oder TIF-Format.
**  Das IMG-Format wird von den Programmen IRIS, IS30, ... im Institut
**  für ExPhysik der HHUD benutzt.
**
**  18.02.1994  Beginn
**  02.03.1994  IMG-Format mit 512 Zeilen aufgenommen
**  03.03.1994  GIF-Format Version 89a aufgenommen
**  05.03.1994  glätten der Grauwerte aufgenommen
**  07.04.1994  Defaults für Extensions der Dateinamen und den 2. Dateinamen
**              aufgenommen
**  28.04.1994  Schalterauswertung in dem 2xxx-Modulen
**  07.12.1994  Datum des Output-Files wird auf das Datum des IMG-Files gesetzt
*/

#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <io.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "img2x.h"

/********************************************************************/

/*
**  Kurzbeschreibung des Programms
*/
static const char manual [] =
  "%s     .02.1994     Ulrich Berntien\n"
  "Institut für Experimentalphysik der HHUD\n"
  "\n"
  "Aufrufformat:\n"
  "   %s [schalter] <IMG-File> [<Output-File>]\n"
  "\n"
  "Das Bild aus dem IMG-File wird gelesen und in das neu erstellte\n"
  "Output-File übersetzt.\n"
  "Es werden IMG-Files mit 256 oder 512 Zeilen pro Bild mit 512 Pixel pro\n"
  "Zeile gelesen. Die IMG-Files haben keinen Header\n"
  "Wird im <IMG-File> keine Extension angegeben (kein '.' im Namen), so\n"
  "wird die Extension \".IMG\" benutzt\n"
  "Wird im <Output-File> keine Extension angegeben, so wird die\n"
  "Extension \"%s\" benutzt. Wird kein <Output-File> angegeben, so wird\n"
  "der Mame <IMG-File> mit dieser Extension benutzt.\n"
  "Beginnt ein Filename mit dem Zeichen '-', so muß es zu '--' verdoppelt\n"
  "werden damit die Unterscheidung von einem Schalter möglich ist.\n"
  "Existiert das Ausgabefile bereits, so wird eine Kontrollabfrage\n"
  "durchgeführt. Dieses Verhalten wird durch die Schalter -j/-n geändert.\n"
  "%s"
  "mögliche Schalter:\n"
  "  -?  Ausgabe dieses Textes.\n"
  "  -v  zwischen dem kleinsten benutzten Grauwert und dem größten benutzten\n"
  "      Grauwert des IMG-Files werden linear die RGB-Werte von (0,0,0) bis\n"
  "      (255,255,255) zugeordnet.\n"
  "  -s  es werden nur den benutzten Grauwerten die RGB-Werte von (0,0,0)\n"
  "      bis (255,255,255) zugeordnet.\n"
  "  ohne einen Schalter -v oder -s :\n"
  "      Den Grauwerten 0 bis 255 werden die RGB-Werte (0,0,0) bis\n"
  "      (255,255,255) zugeordnet.\n"
  "  -1  Keine Kompression des Output-Files.\n"
  "  -g  leichtes Glätten der Grauwerte. Das niederwertigste Bit aller\n"
  "      Grauwerte wird auf 0 gesetzt.\n"
  "  -G  stärkeres Glätten der Grauwerte. Die beiden niederwertigsten Bits\n"
  "      aller Grauwerte werden auf 0 gesetzt.\n"
  "  -j  automatisch existierende Dateien überschreiben.\n"
  "  -n  automatisch existierende Dateien nicht überschreiben.\n"
  "";

/** Fehlermeldungen *************************************************/

/*
**  Fehlermeldung ausgeben und Programm benenden
**  format : Format für fprintf
*/
void error ( const char* format, ... )
  {
  va_list list;
  va_start( list, format );
  vfprintf( stderr, format, list );
  va_end( list );
  exit( EXIT_FAILURE );
  }

/*
**  Fehler durch falsche Angabe in der Kommandozeile aufgetreten, Programmende
**  format : Format für vfprintf
*/
static void cmderror ( const char* format, ... )
  {
  va_list list;
  fputs( "Eingabefehler: ", stderr );
  va_start( list, format );
  vfprintf( stderr, format, list );
  va_end( list );
  fprintf( stderr, "\nWeiter Informationen mit: %s -?\n", progname );
  exit( EXIT_FAILURE );
  }

/*
**  Fehlermeldung und Programmende nach Problem mit einem File
**  fname  : Name des Files
**  action : Bei diese Aktion trat der Fehler auf
*/
static void fileerror ( const char* fname, const char* action )
  {
  fprintf( stderr, "Fehler beim %s von File \"%s\".\n"
                   "(Laufzeitsystem meldet: %s)\n",
           fname, action, strerror( errno ) );
  exit( 1 );
  }

/*
**  Fehlermeldung und Programmende, weil ein interner Fehler aufgetreten
**  fktname : Name der Funktion
**  msg     : Fehlermeldung
*/
void internerror ( const char* fktname, const char* msg )
  {
  error( "interner Fehler in Funktion %s() aufgetreten:\n"
         "%s\n", fktname, msg );
  }

/** Basisfunktionen *************************************************/

/*
**  wie malloc, aber mit Fehlermeldung
**  size   : Größe des benötigten Speichers auf dem Heap in Bytes
**  return : Zeiger auf den Speicherblock (nie NULL)
*/
void* Malloc ( size_t size )
  {
  void *const result = malloc( size );
  if( result == NULL )
    error( "(verwalteter) Hauptspeicher zu klein. {malloc(%u)}.\n", size );
  return result;
  }

/*
**  wie lseek, aber mit Fehlerbehandlung
**  file     : Seek-Operation auf dieses File ausführen
**  position : R/W-Pointer auf diese Position (von Fileanfang) setzen
**  filename : Name des Files für Fehlermeldung
*/
void Lseek ( int file, long position, const char* filename )
  {
  if( lseek( file, position, SEEK_SET ) != position )
    fileerror( "R/W-Pointer setzen", filename );
  }

/** IMG-File lesen **************************************************/

/*
**  Höhe des Bild in Zeilen 256 oder 512
**  die Entscheidung wird über die Dateigröße getroffen
*/
int img_height;

/*
**  Aufbau des IMG-Files:
**    Das Bild wird zeilenweise abgespeichert.
**    Die höchste Zeile des Bilds zu erst.
**    Die Pixel in einer Zeile werden von Links nach Rechts gespeichert.
**    Jedes Pixel wird in einem Byte (8 Bit) gespeichert.
**    Hinter den Bilddaten folgen zusätzliche Informationen (z.B.
**    Datum und Uhrzeit der Aufnahme) diese werden aber hier ignoriert.
*/

/*
**  Bitmaske für eine mögliche Glättung der Grauwerte
*/
static BYTE img_glaettung = 0xff;

/*
**  Name des Files merken für Fehlermeldungen
*/
static const char* img_filename = NULL;

/*
**  Filehandle für Zugriffe auf das File (read binary)
*/
int img_file = -1;

/*
**  Buffer für Zwischenspeichern der Bildzeilen aus dem File
*/
BYTE* img_buffer = NULL;

/*
**  Anzahl der Bytes im Buffer
*/
size_t img_bufferused;

/*
**  nächste Leseposition aus dem Buffer
*/
size_t img_readpointer;

/*
**  Anzahl der noch zu lesenden Zeilen
*/
int img_zeile;

/*
**  Tabelle der benutzten Grauwerte, wird während des Lesens
**  aus dem IMG-file aufgebaut
*/
static char img_used [0x100];

/*
**  Fehlermeldung und Programmende bei Problemen mit dem IMG-File
**  action : bei dieser Aktion ist der Fehler aufgetreten
*/
void img_error ( const char* action )
  {
  fileerror( action, img_filename );
  }

/*
**  Schließen des IMG-Files
*/
void img_close ( void )
  {
  if( img_file > -1 )
    {
    if( close( img_file ) != 0 ) img_error( "Schließen" );
    free( img_buffer );
    img_file = -1;
    img_buffer = NULL;
    }
  }

/*
**  Öffnen des IMG-Files
**  gloable Variable 'img_filename' muß gesetzt sein
*/
static void img_open ( void )
  {
  img_close();
  img_buffer = Malloc( img_bufferlines * img_width );
  img_bufferused = 0;
  img_file = dos_open( img_filename, O_RDONLY );
  if( img_file < 0 ) img_error( "Öffnen" );
  if( (long) img_width * 300 < filelength( img_file ) )
    img_height = 512;
  else
    img_height = 256;
  memset( img_used, 0, sizeof img_used );
  img_zeile = img_height;
  printf( "Lese aus IMG-File \"%s\" mit %d Zeilen.\n",
          img_filename, img_height );
  img_fillbuffer();
  }

/*
**  Seek-Operation auf dem img_file ausführen mit Fehlerkontrolle
**  offset : Read/Write Pointer auf diese Position setzen
*/
void img_seek ( long offset )
  {
  Lseek( img_file, offset, img_filename );
  }

/*
**  Zuordnung dem Grauwert x den RGB-Wert (y,y,y) = (x,x,x)
**  tabelle : in dieser Tabelle wird der Wert y gespeichert
*/
static void img_normalscale ( BYTE tabelle [0x100] )
  {
  unsigned i;
  for( i = 0; i < 0x100; ++i ) tabelle[i] = (BYTE) i;
  }

/*
**  Zurordnung dem Grauwert x den RGB-Wert (y,y,y)
**  y = 256 * ( x - minium(x) ) / ( maximum(x) - minimum(y) )
**  tabelle : in dieser Tabelle wird der Wert y gespeichert
*/
static void img_minmaxscale ( BYTE tabelle [0x100] )
  {
  unsigned i;
  unsigned min;
  unsigned max;
  unsigned range;
  for( min = 0; min < 0x100 && img_used[min] == 0; ++min ) {}
  for( max = 0xff; max > 0 && img_used[max] == 0; --max ) {}
  range = max - min;
  for( i = 0; i < min; ++i ) tabelle[i] = 0;
  for( i = min; i <= max; ++i ) tabelle[i] = ( ( i - min ) * 255 )/ range;
  for( i = max + 1; i < 0x100; ++i ) tabelle[i] = 255;
  }

/*
**  Zuordung dem Grauwert x den RGB-Wert (y,y,y)
**  Wert y von 0 bis 255 linear über die benutzten x Werte
**  tabelle : in dieser Tabelle wird der Wert y gespeichert
*/
static void img_stufenscale ( BYTE tabelle [0x100] )
  {
  unsigned i;
  unsigned stufe = 0;
  unsigned wert = 0;
  unsigned count = 0;
  for( i = 0; i < 0x100; ++i ) count += img_used[i];
  if( count > 1 ) --count;
  for( i = 0; i < 0x100; ++i )
    {
    if( img_used[i] )
      wert = ( stufe++ * 255 ) / count;
    tabelle[i] = wert;
    }
  }

/*
**  Ermittlung der Tabelle Grauwert x - RGB-Wert (y,y,y)
**  art    : Art der Umwandlung Grauwert - RGB-Wert
**  return : die Tabelle der Werte y indiziert durch die Grauwerte x
**           (free nicht vergessen)
*/
BYTE* img_getgrayscale ( img_skalierung art )
  {
  BYTE *const tabelle = Malloc( 0x100 );
  switch( art )
    {
    case _img_normal : img_normalscale( tabelle );
                       break;
    case _img_minmax : img_minmaxscale( tabelle );
                       break;
    case _img_stufen : img_stufenscale( tabelle );
                       break;
    default          : internerror( "img_getgrayscale", "art ungültig" );
    }
  return tabelle;
  }

/*
**  Die Grauwerte im Buffer 'img_buffer' in der Tabelle 'img_used' registrieren
**  und ggf. glätten
*/
void img_registriere ( void )
  {
  size_t i;
  if( img_glaettung == 0xFF )
    for( i = 0; i < img_bufferused; ++i ) img_used[ img_buffer[i] ] = 1;
  else
    for( i = 0; i < img_bufferused; ++i )
      img_used[ img_buffer[i] &= img_glaettung ] = 1;
  }

/** Ausgabefile schreiben *******************************************/

/*
**  Größe des Ausgabebuffers
*/
#define out_buffersize (48*1024)

/*
**  Name des OUT-Files für Fehlermeldungen
*/
const char* out_filename = NULL;

/*
**  Filehandle für Zugrifff auf das OUT-File (write only)
*/
static int out_file = -1;

/*
**  Schreibbuffer
*/
static BYTE* out_buffer;

/*
**  nächste Schreibposition in den Buffer 'out_buffer'
*/
static size_t out_writepointer;

/*
**  Fehlermeldung und Programmende bei Problem mit dem OUT-File
**  action : bei dieser Aktion ist das Problem aufgetreten
*/
static void out_error ( const char* action )
  {
  fileerror( action, out_filename );
  }

/*
**  Leeren des Schreibbuffers, setzt den Schreibpointer 'out_writepointer'
*/
static void out_writebuffer ( void )
  {
  if( out_writepointer > 0 )
    {
    if( write( out_file, out_buffer, out_writepointer ) != out_writepointer )
      out_error( "Schreiben" );
    out_writepointer = 0;
    }
  }

/*
**  Schließen des OUT-Files
*/
void out_close ( void )
  {
  if( out_file > -1 )
    {
    out_writebuffer();
    if( close( out_file ) != 0 ) out_error( "Schließen" );
    out_file = -1;
    free( out_buffer );
    }
  }

/*
**  Erzeugen des OUT-Files
**  globale Variable 'out_filename' muß gestzt sein
*/
static void out_open ( void )
  {
  out_file = dos_creat( out_filename, 0 );
  if( out_file < 0 ) out_error( "Anlegen/Öffnen" );
  out_buffer = Malloc( out_buffersize );
  out_writepointer = 0;
  }

/*
**  Schreiben in den 'out_buffer'
**  size   : Anzahl der zu schreibenden Bytes
**  buffer : der Buffer mit den zu schreibenden Bytes
*/
void out_write ( size_t size, const void* buffer )
  {
  if( size > out_buffersize ) internerror( "out_write", "Buffer zu klein" );
  if( size > out_buffersize - out_writepointer ) out_writebuffer();
  memcpy( out_buffer + out_writepointer, buffer, size );
  out_writepointer += size;
  }

/*
**  Buffer für das Schreiben in das OUT-File anfordern
**  return : Zeiger auf den Buffer, bleibt bis zum nächsten Aufruf von
**           out_getbuffer(), out_write() oder out_filelength() gültig.
*/
void* out_getbuffer ( size_t size )
  {
  void* result;
  if( size > out_buffersize ) internerror( "out_getbuffer", "Buffer zu klein" );
  if( size > out_buffersize - out_writepointer ) out_writebuffer();
  result = out_buffer + out_writepointer;
  out_writepointer += size;
  return result;
  }

/*
**  Rückgabe nicht angeforderter aber nicht benötigter Bytes
**  size : Anzahl der nicht benötigten Bytes
*/
void out_ungetbuffer ( size_t size )
  {
  if( size > out_writepointer )
    internerror( "out_ungetbuffer", "Buffer unterlaufen" );
  out_writepointer -= size;
  }

/*
**  Schreibt eine Byte in das OUT-File (über den out_buffer)
**  byte : das zu schreibende Byte
*/
void out_writebyte ( BYTE byte )
  {
  if( out_writepointer >= out_buffersize ) out_writebuffer();
  out_buffer[ out_writepointer++ ] = byte;
  }

/*
**  lseek zu absoluter Position im OUT-File mit vorherigem Schreiben
**  des out_buffers
**  offset : Position im File
*/
void out_seek ( long offset )
  {
  out_writebuffer();
  Lseek( out_file, offset, out_filename );
  }

/*
**  Bestimmt die aktuelle Länge des OUT-Files
**  Es gibt Probleme beim Zusammenspiel der lseek() Funktionsaufrufe
**  daher muß die Datei neu geöffnet werden.
**  return : aktuelle Länge des OUT-Files in Byte
*/
long out_filelength ( void )
  {
  static const char fktname [] = "filelength";
  long position;
  long result;
  out_writebuffer();
  position = lseek( out_file, 0L, SEEK_CUR );
  result = lseek( out_file, 0L, SEEK_END );
  close( out_file );
  out_file = dos_open( out_filename, O_WRONLY );
  if( out_file < 0 ) internerror( fktname, "kann File nicht reopen" );
  if( position != lseek( out_file, position, SEEK_SET ) )
    internerror( fktname, "seek gescheitert" );
  return result;
  }

/*
**  Setzt das Datum/Uhrzeit des Ausgabefiles auf das Datum/Uhrzeit
**  des IMG-Files
*/
static void out_setdate ( void )
  {
  unsigned date;
  unsigned time;
  if( dos_getftime( img_file, &date, &time ) == 0 )
    dos_setftime( out_file, date, time );
  }

/** MAIN ************************************************************/

/*
**  Default Extension für den Namen des IMG-Files
*/
static const char extensionimg [] = ".IMG";

/*
**  Soll das Bild komprimiert gespeichert werden ?
**  Standard-Wert : Ja
*/
static int kompression = 1;

/*
**  Wie sollen die Grauwerte in RGB-Werte übersetzt werden ?
**  Standard-Wert : normal, Grauwert x in RGB-Wert (x,x,x)
*/
static img_skalierung skala = _img_normal;

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
      case +1 : fprintf( stderr, "Output-File \"%s\" existiert.\n", filename );
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
  char *const result = Malloc( namelen + 1 + strlen(extension) );
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
**  Auswerten eines Schalters von der Kommandozeile
**  sw : der Schalter ohne das Schaltersymbol
*/
static void schalter ( const char* sw )
  {
  static const char format [] = "Unbekannter Schalter \"%s\".";
  if( sw[1] != '\0' )
    {
    if( ! schalter2( sw ) ) cmderror( format, sw );
    }
  else
    switch( sw[0] )
      {
      case 'v' : skala = _img_minmax;
                 break;
      case 's' : skala = _img_stufen;
                 break;
      case '1' : kompression = 0;
                 break;
      case 'g' : img_glaettung = 0xFF - 1;
                 break;
      case 'G' : img_glaettung = 0xFF - 3;
                 break;
      case 'j' : fileschutz = -1;
                 break;
      case 'n' : fileschutz = +1;
                 break;
      case '?' : printf( manual, progname, progname, extension, progfunktion );
                 if( manual2 != NULL ) puts( manual2 );
                 exit( 0 );
      default  : if( ! schalter2( sw ) ) cmderror( format, sw );
      }
  }

/*
**  M A I N
*/
int main ( int argc, char* argv [] )
  {
  if( argc < 2 ) cmderror( "Keine Argumente angegeben." );
  while( ++argv, --argc > 0 )
    if( (*argv)[0] == '-' && (*argv)[1] != '-' )
      schalter( *argv + 1 );
    else
      {
      const int offset = (*argv)[0] == '-' ? 1 : 0;
      if( img_filename == NULL )
        img_filename = *argv + offset;
      else if( out_filename == NULL )
        out_filename = *argv + offset;
      else
        cmderror( "Dritter Filename \"%s\" angegeben.", *argv + offset );
      }
  if( img_filename == NULL ) cmderror( "IMG-File nicht angegeben" );
  if( ! extensionexist( img_filename ) )
    img_filename = extensionadd( img_filename, extensionimg );
  if( out_filename == NULL )
    out_filename = extensionadd( img_filename, extension );
  else
    if( ! extensionexist( out_filename ) )
      out_filename = extensionadd( out_filename, extension );
  filecontrol( out_filename );
  img_open();
  out_open();
  trafo( skala, kompression );
  out_setdate();
  out_close();
  img_close();
  return EXIT_SUCCESS;
  }
