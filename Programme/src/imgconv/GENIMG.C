/*
**  Zortech C V2.1
**  .02.1994  Ulrich Berntien
**
**  Erzeugen von Testbildern im IMG-Format (Bildformat des Programms
**  IRIS, IS30 und anderer Bildverarbeitungsprogramme im Inst. ExPhysik).
**
**  18.02.1994  Beginn
**  02.03.1994  IMG-Format mit 512 Zeilen aufgenommen
**  25.08.1994  Fehler in gen_linksrechts() behoben
*/

#include <dos.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
**  Stackgröße auf 6 KByte setzen, für die lokalen Bildzeilen-Buffer
*/
unsigned int _stack = 6 * 1024;

/********************************************************************/

/*
**  Kurzbeschreibung des Programms
*/
static const char manual [] =
  "GENIMG   .02.1994  Ulrich Berntien\n"
  "Institut für Experimentalphysik der HHUD\n"
  "\n"
  "Erstellen von Testbildern im IMG-Format.\n"
  "\n"
  "Aufrufformat:\n"
  "  GENIMG <Ziffer>[v] <IMG-File>\n"
  "Das Testbild wird in <IMG-FILE> gespeichert. Der Inhalt\n"
  "des Testbilds wird durch die <Ziffer> bestimmt.\n"
  " Nummer   Inhalt\n"
  "   1      Zunehmender Grauwert von Oben nach Unten\n"
  "   2      Zunehmender Grauwert von Links nach Rechts\n"
  "   3      Gittermuster\n"
  "   4      vertikale Streifen mit Grauwert 30 - 93\n"
  "   5      Rauschen, Grauwerte über Zufallszahl-Generator\n"
  "Wird der Buchstabe 'v' an die Ziffer angehängt, so wird\n"
  "ein Bild mit 512 Zeilen erzeugt, sonst 256 Zeilen.\n"
  "\n";

/********************************************************************/

/*
**  Ein Pixel im IMG-Format wird als ein Byte dargestellt
*/
typedef unsigned char byte;

/*
**  Anzahl der Pixel pro Zeile beim IMG-Format
**  Anzahl der Zeilen pro Bild beim IMG-Format 256 oder 512 Zeilen
*/
#define img_breite 512
static int img_hoehe = 256;

/** img Modul *******************************************************/

/*
**  Name des Files
*/
static const char* img_name;

/*
**  Handle des Files (write only)
*/
static int img_file;

/*
**  Aktuelle Zeilennummer, zur Kontrolle der Filegröße
*/
static int img_zeilennummer;

/*
**  Fehlermeldung nach Problemen mit dem File und Programmende
**  action : Aktion bei der der Fehler auftrat
*/
static void img_error ( const char* action )
  {
  fprintf( stderr, "Fehler beim %s in File \"%s\".\n"
                   "(Systemmeldung : %s)\n",
           action, img_name, strerror( errno ) );
  exit( EXIT_FAILURE );
  }

/*
**  Erzeugen des IMG-Files
**  filename : Name des Files
*/
static void img_create ( const char* filename )
  {
  img_name = filename;
  img_zeilennummer = 0;
  img_file = dos_creat( filename, 0 );
  if( img_file < 0 ) img_error( "Öffnen" );
  }

/*
**  Schreiben einer Bildzeile in das IMG-File
**  zeile : die Zeile
*/
static void img_write ( byte zeile [img_breite] )
  {
  ++img_zeilennummer;
  if( write( img_file, zeile, img_breite ) != img_breite )
    img_error( "Schreiben einer Bildzeile" );
  }

/*
**  Schließen des IMG-Files.
**  Die übliche Postfix-Daten (Datum,Uhrzeit,Größe,..) des IMG-Files
**  werden hier nicht geschieben.
**/
static void img_close ( void )
  {
  if( img_zeilennummer != img_hoehe )
    {
    fprintf( stderr, "Die letzte Zeile hat Nummer %d, Fehler.\n", img_zeilennummer );
    exit( EXIT_FAILURE );
    }
  if( close( img_file ) != 0 ) img_error( "Schließen" );
  }

/** gen Modul *******************************************************/

/*
**  Testbild erzeugen.
**  Innerhalb einer Zeile ein konstanter Grauwert; von Oben nach Unten
**  steigt der Grauwert (die Helligkeit der Pixel) an.
*/
static void gen_obenunten ( void )
  {
  byte buffer [img_breite];
  long i;
  for( i = 0; i < img_hoehe; ++i )
    {
    byte akku = (byte) ( ( i * 256 )/ img_hoehe );
    memset( buffer, akku, sizeof buffer );
    img_write( buffer );
    }
  }

/*
**  Testbild erzeugen.
**  Innerhalb einer Spalte ein konstanter Grauwert; von Links nach Rechts
**  steigt der Grauwert (die Helligkeit der Pixel) an.
*/
static void gen_linksrechts ( void )
  {
  byte buffer [img_breite];
  long i;
  for( i = 0; i < img_breite; ++i )
    buffer[i] = (byte) ( ( i * 256 )/ img_breite );
  for( i = 0; i < img_hoehe; ++i )
    img_write( buffer );
  }

/*
**  Testbild erzeugen.
**  Ein Gitter aus je 20 horizontalen und vertikalen Linien.
*/
static void gen_gitter ( void )
  {
  byte hlinie [img_breite];
  byte vlinie [img_breite];
  int i;
  memset( hlinie, 255, sizeof hlinie );
  memset( vlinie, 0, sizeof vlinie );
  for( i = 0; i < img_breite; i += img_breite/20 ) vlinie[i] = 255;
  for( i = 0; i < img_hoehe; ++i )
    img_write( ( i % 20 == 0 ) ? hlinie : vlinie );
  }

/*
**  Testbild erzeugen.
**  Die Grauwerte werden über den Zufallszahl-Generator erzeugt.
**  Der Start der Generators erfolgt über die Uhrzeit.
*/
static void gen_zufall ( void )
  {
  byte buffer [img_breite];
  int i;
  unsigned date, time;
  dos_getftime( img_file, &date, &time );
  srand( time );
  for( i = 0; i < img_hoehe; ++i )
    {
    int j;
    for( j = 0; j < img_breite; ++j ) buffer[j] = (byte) rand();
    img_write( buffer );
    }
  }

/*
**  Testbild erzeugen
**  Die Grauwerte in den Spalten sind konstant.
**  In den Zeilen gibt es Streifen mit den Grauwerten 30 - 93
*/
static void gen_vstreifen ( void )
  {
  byte buffer [img_breite];
  int i;
  for( i = 0; i < img_breite; ++i ) buffer[i] = 30 + ( i & 63 );
  for( i = 0; i < img_hoehe; ++i ) img_write( buffer );
  }

/*
**  Verteilen der Testbild erzeugenden Funktionen auf die Testbild Nummern
**  nummer : Nummer des gewünschten Testbilds (1..4)
*/
static void generate ( int nummer )
  {
  switch( nummer )
    {
    case  1 : gen_obenunten();
              break;
    case  2 : gen_linksrechts();
              break;
    case  3 : gen_gitter();
              break;
    case  4 : gen_vstreifen();
              break;
    case  5 : gen_zufall();
              break;
    default : fprintf( stderr, "Bildnummer %d nicht bekannt.\n", nummer );
              exit( EXIT_FAILURE );
    }
  }

/********************************************************************/

/*
**  Fehlermeldung ausgeben und Programm beenden
**  msg : die Fehlermeldung
*/
static void error ( const char* msg )
  {
  fprintf( stderr, "GEN-IMG Fehler : %s"
                   "Programmkurzbeschreibung mit Aufruf \"GENIMG\".\n",
                   msg );
  exit( EXIT_FAILURE );
  }

/*
**  Auswahl der Nummer und des Formats
**  str    : der String in der Form <Ziffer>[v]
**  return : Wert der Ziffer
*/
static int auswahl ( const char* str )
  {
  const int nummer = str[0] - '0';
  if( str[1] == '\0' )
    img_hoehe = 256;
  else if( str[2] == '\0' )
    {
    if( str[1] != 'v' )  error( "nur Buchstabe 'v' erlaubt.\n" );
    img_hoehe = 512;
    }
  else
    error( "nur <Ziffer> oder <Ziffer>v erlaubt\n" );
  return nummer;
  }

/*
**  M A I N
*/
int main ( int argc, char* argv [] )
  {
  if( argc != 3 )
    {
    fputs( manual, stderr );
    exit( EXIT_FAILURE );
    }
  img_create( argv[2] );
  generate( auswahl( argv[1] ) );
  img_close();
  return EXIT_SUCCESS;
  }
