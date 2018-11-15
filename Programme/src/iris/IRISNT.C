/*
**  C   ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  .03.1993  Ulrich Berntien
**
**  Ausgeben der Dichte/Temperatur-Verteilungen des Moduls 'iris04.c'
**  im Programm 'is30'
**
**  Ausgabe für HP-LaserJet II
**
**  08.03.1993  Beginn
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "plot.h"
#include "irisnt.h"

#ifndef _far
#  define _far    far
#  define _near   near
#  define _cdecl  cdecl
#  define _pascal pascal
#endif

#define LOCAL _near _pascal
#define TEXT static const char

#define ESC "\x1B"

/********************************************************************/

/*
**  Kurzbeschreibung des Programms
*/
TEXT manual [] =
  "IRIS-N-T    .03.1993 Ulrich Berntien\n"
  "Ausgabe der Dichteverteilung n(x) und der Temperaturverteilung T(x)\n"
  "über dem Spalte des Spektrometers (x)\n\n"
  "Aufrufformat\n\n"
  "  IRISNT inputfile [outputfile]\n\n"
  "inputfile  : Name des Eingabefiles\n"
  "             Enthält die von IRIS04 (IS30) erzeugten Werte.\n"
  "outputfile : Vorgabe 'PRN'\n"
  "             Schreibt Steuercodes für HP-LaserJet.\n";

/*
**  Steuercode-Sequenzen für den HP-LJ Drucker
*/
TEXT prn_init [] =
  ESC "E"               /* reset */
  ESC "(10U"            /* IBM-Zeichensatz */
  ESC "&a5L";           /* linker Druckrand auf Spalte 5 setzen */

TEXT prn_termit [] =
  ESC "E";              /* reset */

TEXT prn_fett [] =
  ESC "(s3B";           /* Strichbreite +3 Einheiten */

TEXT prn_mittel [] =
  ESC "(sB";            /* Steichbreite +-0 Einheiten */

TEXT prn_push [] =
  ESC "&f0S";           /* Cursor-Position speichern */

TEXT prn_pop [] =
  ESC "&f1S";           /* Cursor-Position abrufen */

TEXT prn_rand2mitte [] =
  ESC "&a45L";          /* Linker Druckrand auf Spalte 45 setzen */

TEXT prn_rand2links [] =
  ESC "&a5L";           /* Linker Druckrand auf Spalte 5 setzen */

/*
**  Bildunterschriften
*/
TEXT txt_dichte [] =
  "HOR. von Zeile %d bis Zeile %d\r\n"
  "VER. Elektronendichte / cm^-3\r\n"
  "      von %5.0le bis %5.0le";

TEXT txt_temperatur [] =
  "HOR. von Zeile %d bis Zeile %d\r\n"
  "VER. Elektronentemperatur / eV\r\n"
  "      von %5.0le bis %5.0le";

/********************************************************************/

/*
**  Fehlermeldung ausgeben und Programm beenden
**  msg : Meldung
*/
static void LOCAL error ( const char* msg )
  {
  fputs( "IRIS-N-T : Fehler ", stderr );
  fputs( msg, stderr );
  fputc( '\n', stderr );
  exit( 1 );
  }

/*
**  Schreibt String in File mit Fehlerkontrolle
**  str  : der String
**  file : das File
*/
static void LOCAL fstring( const char* str, FILE* file )
  {
  if( fputs( str, file ) )
    error( "beim Schreiben eines Strings ins Ausgabe-file" );
  }

/*
**  Ausgeben in File mit Fehlerkontrolle
**  file : das File
**  form : Format (Ausgabe erfolgt über vfprintf)
*/
static void fprint ( FILE* file, const char* form, ... )
  {
  va_list arg_ptr;
  va_start( arg_ptr, form );
  if( vfprintf( file, form, arg_ptr ) < 0 )
    error( "beim Schreiben ins Ausgabefile" );
  va_end( arg_ptr );
  }

/*
**  Bestimmt Minimum und Maxiumum der Werte
**  count : Anzahl der Werte
**  werte : die Zahlwerte selbst
**  pmin  : dorthin wird der minimale Wert geschreiben
**  pax   : dorthin wird der maximale Wert geschrieben
*/
static void minmax ( int count, const double* werte, double* pmin, double* pmax )
  {
  double min = *werte;
  double max = *werte;
  int i;
  for( i = 1; i < count; ++i )
    if( min > werte[i] )
      min = werte[i];
    else if( max < werte[i] )
      max = werte[i];
  *pmax = max;
  *pmin = min;
  }

static void rounddown( double* );   /* forward declaration */

/*
**  Wert nach oben Runden
**  ptr : Zeiger auf den Wert
*/
static void roundup ( double* ptr )
  {
  if( *ptr < 0.0 )
    {
    *ptr *= -1;
    rounddown( ptr );
    *ptr *= -1;
    }
  if( *ptr > 0.0 )
    {
    const double akku = pow( 10.0, floor( log10(*ptr) ) );
    *ptr = ceil( *ptr / akku ) * akku;
    }
  }

/*
**  Wertnach unten Runden
**  ptr : Zeiger auf den Wert
*/
static void rounddown ( double* ptr )
  {
  if( *ptr < 0.0 )
    {
    *ptr *= -1;
    roundup( ptr );
    *ptr *= -1;
    }
  if( *ptr > 0.0 )
    {
    const double akku = pow( 10.0, floor( log10(*ptr) ) );
    *ptr = floor( *ptr / akku ) * akku;
    }
  }

/*
**  Grafik mit Bildunterschrift ausgeben
**  file  : das Ausgabefile, write binary
**  daten : die Angaben über Zeilenzahlen
**  werte : die auszuplottenden Werte
**  text  : Textform für die Beschriftung
*/
static void grafik ( FILE* file, const ntspeicher* daten,
                     const double* werte, const char* text )
  {
  const int punkte = ( daten->bis - daten->von ) / 2;
  double unten, oben, faktor;
  unsigned char* plotdata = malloc( punkte );
  int i;
  if( plotdata == NULL )
    error( "Out of Heap (in grafik)" );
  minmax( punkte, werte, &unten, &oben );
  roundup( &oben );
  rounddown( &unten );
  if( ( oben - unten ) < 1e-30 )
    {
    oben *= 1.10;
    roundup( &oben );
    unten *= 0.90;
    rounddown( &unten );
    }
  faktor = 256.0 / ( oben - unten );
  for( i = 0; i < punkte; ++i )
    plotdata[i] = (unsigned char) ( ( werte[i] - unten ) * faktor );
  if( plot( file, plotdata, punkte, 1 ) )
    error( "von plot gemeldet" );
  fprint( file, text,
          daten->von, daten->bis, unten, oben );
  free( plotdata );
  }

/*
**  Fügt wenn notwendig einen Seitenvorschub ein
**  file : Das Ausgabefile, write binary
*/
static void LOCAL neueseite ( FILE* file )
  {
  static int count = 0;
  if( ++count > 4 )
    {
    fstring( "\f", file );
    count = 1;
    }
  }

/*
**  Bearbeitet einen Datensatz schreibt Ausdruck-Daten in das Ausgabefile
**  daten  : der Datensatz
**  output : das Ausgabefile, write binary
*/
static void LOCAL process ( const ntspeicher* daten, FILE* output )
  {
  const int count = ( daten->bis - daten->von ) / 2;
  printf( "Ausgabe von '%s'\n", daten->name );
  if( count < 1 || count > 256 )
    printf( "Anzahl der Punkte %d ungültig.\n", count );
  else
    {
    neueseite( output );
    fprint( output, "%s%s%s ,gemittelt über %d (%d) Zeilen, ",
                    prn_fett, daten->name, prn_mittel,
                    daten->mittelueber, daten->mittelueber * 2 );
    switch( daten->art )
      {
      case 'a' : fstring( "Alpha", output );
                 break;
      case 'b' : fstring( "Beta", output );
                 break;
      case 'c' : fstring( "Gamma", output );
                 break;
      default  : fprint( output, "%c ", daten->art );
      }
    fprint( output, "-Linie\r\n%s    ", prn_push );
    grafik( output, daten, daten->n, txt_dichte );
    fprintf( output, "%s%s    ", prn_pop, prn_rand2mitte );
    grafik( output, daten, daten->t, txt_temperatur );
    fprint( output, "%s\r\n", prn_rand2links );
    }
  }

/*
**  Öffnet Eingabefile ( read binary )
**  fname  : Name des Files
**  return : das geöffnete File
*/
static FILE* LOCAL openinput ( const char* fname )
  {
  FILE* file = fopen( fname, "rb" );
  printf( "lese Plot-Daten aus File : %s\n", fname );
  if( file == NULL )
    error( "Kann File nicht zum Lesen öffnen." );
  return file;
  }

/*
**  Öffnet Ausgabefile ( write binary )
**  fname  : Name des Files
**  return : das geöffnete File
*/
static FILE* LOCAL openoutput ( const char* fname )
  {
  FILE* file = fopen( fname, "wb" );
  printf( "schreibe Druckdaten in File : %s\n", fname );
  if( file == NULL )
    error( "Kann File nicht zum Schreiben öffnen." );
  return file;
  }

/*
**  M A I N
*/
int main ( int argc, char* argv [] )
  {
  FILE* input;
  FILE* output;
  ntspeicher* daten;

  if( argc != 2 && argc != 3 )
    {
    fputs( manual, stderr );
    exit( 1 );
    }
  plot_init();
  plot_div( 50, 32 );

  if( ( daten = malloc( sizeof (ntspeicher) ) ) == NULL )
    error( "Out of Heap (in main)" );
  input = openinput( argv[1] );
  output = openoutput( argc > 2 ? argv[2] : "PRN" );

  fstring( prn_init, output );
  while( fread( daten, sizeof (ntspeicher), 1, input ) == 1 )
    process( daten, output );
  fstring( prn_termit, output );

  free( daten );
  fclose( input );
  fclose( output );
  return 0;
  }

