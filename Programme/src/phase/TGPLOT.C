/*
**  C   ( Zortech V V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  .03.1993  Ulrich Berntien
**
**  Ausgeben von Textzeilen und Plot-Grafiken
**
**  Ausgabe für HP-LaserJet II
**
**  22.03.1993  Beginn, entstanden aus IRISNT.C
**  29.04.1993  TG00.C und TG01.c einbinden
**  18.08.1993  Modul GETDIR.C eingebunden
**  23.08.1993  eine lange Plot-Grafik in Teilen untereinander ausgeben
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "plot.h"
#include "tgp.h"
#include "tg.h"

#define ESC "\x1B"

/*
**  Blattlänge in Punkte, bei 300 dpi, 6 lpi => 50 dpl, 65 Zeilen pro Blatt
*/
#define BLATT 65 * 50

/*
**  Anzahl der Werte, die in einen Plot passen
*/
#define PLOTLEN 1300

/*
**  Kurzbeschreibung des Programms
*/
static const char manual [] =
  "TGPLOT    .03.1993    Ulrich Berntien\n"
  "Ausgabe von Text und Plots\n"
  "Aufrufformat\n\n"
  "  TGPLOT [-o<outputfile>] inputfile(s)\n\n"
  "inputfile  : Name des Eingabefiles (kann Joker *,? enthalten)\n"
  "outputfile : Vorgabe 'PRN'\n"
  "             Schreibt Steuercodes für HP-LaserJet.\n";

/*
**  Name des Ausgabefiles
*/
static const char* outfile = "PRN";

/*
**  Einleitender Text bei Fehlermeldungen
*/
static const char oneerr [] = "Fehler %s.\n";

/*
**  Steuercode-Sequenzen für den HP-LJ Drucker
*/
static const char prn_init [] =
  ESC "E"         /* reset                    */
  ESC "&l70P"     /* 70 Zeilen pro Seite      */
  ESC "&a5L"      /* linker Rand auf Spalte 5 */
  ESC "(10U";     /* IBM-Zeichensatz          */

static const char prn_termit [] =
  ESC "E";        /* reset */

/********************************************************************/

/*
**  Fehlermeldung ausgeben und Programm beenden
**    form : Ausgabe erfolgt über vfprintf
*/
void error ( const char* form, ... )
  {
  va_list arg_ptr;
  va_start( arg_ptr, form );
  fputs( "\nTGPLOT-Fehler : ", stderr );
  vfprintf( stderr, form, arg_ptr );
  va_end( arg_ptr );
  exit( 1 );
  }

/*
**  Grafikzeilen noch auf der Seite verfügbar
*/
static int pagerest = BLATT;

/*
**  ggf. einen Seitenvorschub
**    punkte : benötigte Anzahl von Zeilen bei 300 dpi
*/
static void LOCAL page ( int punkte )
  {
  if( pagerest < punkte )
    {
    out_puts( "\f" );
    pagerest = BLATT;
    }
  pagerest -= punkte;
  }

/*
**  einen Seitenvorschub ausführen
**    data : das auszugebende Form feed
*/
static void LOCAL plotff ( ff* data )
  {
  UNUSED( data );
  if( pagerest < BLATT )
    {
    out_puts( "\f" );
    pagerest = BLATT;
    }
  }

/*
**  Eine Textzeile ausgeben
**    data : die auszugebende Zeile
*/
static void LOCAL plotzeile ( zeile* data )
  {
  page( 50 );
  out_puts( data->data );
  out_puts( "\r\n" );
  }

/*
**  Einen Graf ausgeben
**    data : der auszugebende Graf
*/
static void LOCAL plotgraf ( graf* data )
  {
  int rest = data->count;
  const unsigned char* ptr = data->data;
  const int i = ( rest + PLOTLEN - 1 )/ PLOTLEN;
  const int len = ( rest + i - 1 )/ i;
  while( rest > 0 )
    {
    const int teil = rest > len ? len : rest;
    page( 512 + 50 );
    if( plot( out_getfile(), ptr, teil, 1 ) )
      error( oneerr, "beim Plotten aufgetreten" );
    ptr += teil;
    rest -= teil;
    }
  }

/*
**  Ein File ausplotten
**  fname : Name des Files
*/
static void LOCAL makeplot ( const char* fname )
  {
  enum tgp inhalt;
  plotfile_open( fname );
  plot_init();
  out_puts( prn_init );
  while( ( inhalt = plotfile_next() ) != _tgp_eof )
    switch( inhalt )
      {
      case _tgp_zeile : plotzeile( plotfile_getzeile() );
                        break;
      case _tgp_graf  : plotgraf( plotfile_getgraf() );
                        break;
      case _tgp_ff    : plotff( plotfile_getff() );
                        break;
      default         : error( oneerr, "Unbekannter Datensatz-Typ" );
      }
  out_puts( prn_termit );
  plotfile_close();
  }

/*
**  Verarbeite einen Filename
**  name : Name des auszugebenden Files, Joker erlaubt
*/
static void LOCAL work ( const char* name )
  {
  const char* filename = dirget_first( name );
  while( filename != NULL )
    {
    makeplot( filename );
    filename = dirget_next();
    }
  }

/*
**  Schalter aus der Kommandozeile verarbeiten
**  sw : der Schalter
*/
static void LOCAL schalter ( const char* sw )
  {
  switch( sw[0] )
    {
    case 'o' : outfile = sw + 1;
	       break;
    default  : error( "unbekennter Schalter : %s\n", sw );
    }
  }

/*
**  M A I N
*/
int main ( int argc, char* argv [] )
  {
  if( argc < 2 ) error( manual );
  if( **(++argv) == '-' )
    {
    schalter( *argv++ + 1 );
    --argc;
    }
  out_open( outfile );
  while( --argc > 0 ) work( *argv++ );
  out_close();
  return 0;
  }

