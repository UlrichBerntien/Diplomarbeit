/*
**  C   ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  .04.1993  Ulrich Berntien
**
**  Ausgeben von Textzeilen und Plot-Grafiken
**
**  Ausgabe in Format für das Einfügen in TeX-Dateien
**
**  Für das Verarbeiten in TeX erwartet die erzeugte Datei
**  die Definition :
**

   \setlength{\unitlength}{26000sp}
   \newsavebox{\TGKoordinaten}

   \savebox{\TGKoordinaten}(1008,256){
     \multiplot(0,0)(0,28){10}{ \line( 1,0){6} }
     \multiput(1008,0)(0,28){10}{ \line(-1,0){6} }
     \multiput(0,0)(100,0){11}{ \line(0, 1){6} }
     \multiput(0,256)(100,0){11}{ \line(0,-1){6} }
     \multiput(0,0)(1008,0){2}{ \line(0,1){256} }
     \multiput(0,0)(0,256){2}{ \line(1,0){1008} }
     }

   % TG{x}{y}{l} - zieht Linien (x,y-l) - (x,y) - (x+1,y)
   \newcommand{\TG}[3]{
     \put(#1,#2){ \line(0,-1){#3} }
     \put(#1,#2){ \line(1,0){1} }
     }

   % TH{x}{y}{l} - zieht Linien (x,y+l) - (x,y) - (x+1,y)
   \newcommand{\TH}[3]{
     \put(#1,#2){ \line(0,1){#3} }
     \put(#1,#2){ \line(1,0){1} }
     }

**
**  29.04.1993  Beginn
*/

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tgp.h"
#include "tg.h"

/*
**  kurze Programmbeschreibung
*/
static const char manual [] =
  "TGTEX     .04.1993     Ulrich Berntien\n"
  "Ausgabe von Plotfile in Format für TeX\n"
  "Aufrufformat :\n\n"
  "    TGTEX <plotfile> <texfile>\n\n"
  "<plotfile> Name des Plotfiles\n"
  "<texfile>  Name des TeX-Files, wird neu erzeugt\n";

/*
**  Ende einer Zeile im TeX-Format bei linksbündigem setzen
*/
static const char eotl []= "\\\\\r\n";

/*
**  Fehlerausgabe und Programm beenden
**    form : Ausgabe erfolgt über vfprintf
*/
void error ( const char* form, ... )
  {
  va_list argptr;
  va_start( argptr, form );
  fputs( "\nTGTeX-Fehler : ", stderr );
  vfprintf( stderr, form, argptr );
  va_end( argptr );
  exit( 1 );
  }

/*
**  Warnung ausgeben, Programm wird normal vortgesetzt
**    form : Ausgabe erfolgt über vfprintf
*/
static void _near warning ( const char* form, ... )
  {
  va_list argptr;
  va_start( argptr, form );
  fputs( "\nTGTeX-Warnung : ", stderr );
  vfprintf( stderr, form, argptr );
  va_end( argptr );
  }

/*
**  Zwischenspeicher für einen String
*/
static const char* onestring = NULL;

/*
**  String in den Zwischenspeicher, falls schon einer im Speicher ist
**  wird dieser ausgegeben
**    str : der String, eine Kopie davon wird gespeichert
*/
static void LOCAL outstring ( const char* str )
  {
  if( onestring != NULL )
    {
    out_puts( onestring );
    out_puts( eotl );
    free( onestring );
    }
  onestring = strdup( str );
  }

/*
**  Entnimmt String aus dem Zwischenspeicher
**    return : Zeiger auf den String (free nicht vergessen) oder NULL
*/
static const char* LOCAL getstring ( void )
  {
  const char* result = onestring;
  onestring = NULL;
  return result;
  }

/*
**  falls String im Buffer diesen Ausgeben
*/
static void LOCAL clearstring ( void )
  {
  if( onestring != NULL )
    {
    out_puts( onestring );
    out_puts( eotl );
    free( onestring );
    onestring = NULL;
    }
  }

/*
**  Umlaut in TeX : " und das Zeichen
**    ptr : Zeiger auf den Buffer
**    c   : das Zeichen
*/
static char* LOCAL clet ( char* ptr, char c )
  {
  *ptr++ = '"';
  *ptr++ = c;
  return ptr;
  }

#ifndef EXIT_SUCCESS

  /*
  **  String copy mit Zeiger zurück hinter das kopierte
  **    dest   : in diesen Buffer kopieren
  **    source : das wird kopiert
  **    return : Zeiger im Buffer hinter das kopierte (auf die '\0')
  */
  static char* LOCAL stpcpy ( char* dest, const char* source )
    {
    int i = 0;
    while( ( dest[i] = source[i] ) != '\0' ) ++i;
    return dest + i;
    }

#endif

/*
**  String zeichenweise für TeX umsetzen
**  dabei TeX Sonderzeichen und deutsche Umlaute beachten
**    str    : der zu übersetzende String
**    return : Übersetzter String (gültig bis nächsten Aufruf)
*/
static const char* chars2tex ( const char* str )
  {
  static char buffer [300];
  char* ptr = buffer;
  char akku;
  int source = 0;
  do{
    switch( akku = str[source++] )
      {
      case '{' :
      case '}' :
      case '#' :
      case '$' :
      case '_' :
      case '%' :
      case '&' : *ptr++ = '\\';  *ptr++ = akku;
                 break;
      case 'ß' : ptr = clet( ptr, 's' );
                 break;
      case 'ü' : ptr = clet( ptr, 'u' );
                 break;
      case 'ö' : ptr = clet( ptr, 'o' );
                 break;
      case 'ä' : ptr = clet( ptr, 'a' );
                 break;
      case 'Ü' : ptr = clet( ptr, 'U' );
                 break;
      case 'Ö' : ptr = clet( ptr, 'O' );
                 break;
      case 'Ä' : ptr = clet( ptr, 'A' );
                 break;
      case '°' : ptr = stpcpy( ptr, "$^\\circ$" );
                 break;
      default  : *ptr++ = akku;
      }
    }
    while( akku != '\0' );
  return buffer;
  }

/*
**  beginnt eine Zahl ?
**    return != 0, wenn c der Anfang einer Zahl sein kann
*/
static int LOCAL startzahl ( char c )
  {
  static const char gueltig [] = "01234546789+-";
  return strchr( gueltig, c ) != NULL;
  }

/*
**  innerhalb einer Zahl ?
**    return != 0, wenn c in einer Zahl sein kann
*/
static int LOCAL inzahl ( char c )
  {
  static const char gueltig [] = "0123456789.,+-eE";
  return strchr( gueltig, c ) != NULL;
  }

/*
**  Zahlen in eimen String bearbeiten für Ausgabe mit TeX
**    str    : der zu berabeitende String
**    return : Übersetzter String (gültig bis nächster Aufruf)
*/
static const char* LOCAL zahlen2tex ( const char* str )
  {
  static char buffer [400];
  char* ptr = buffer;
  int source = 0;
  int zahl = 0;
  int exponent = 0;
  char akku;
  do{
    akku = str[source++];
    if( zahl )
      {
      if( inzahl(akku) )
        {
        if( akku == 'e' || akku == 'E' )
          {
          exponent = 1;
          ptr = stpcpy( ptr, "\\cdot 10^" );
          akku = '{';
          }
        }
      else
        {
        if( exponent )
          {
          exponent = 0;
          *ptr++ = '}';
          }
        zahl = 0;
        *ptr++ = '$';
        }
      }
    else
      if( startzahl(akku) && inzahl(str[source]) )
        {
        zahl = 1;
        *ptr++ = '$';
        }
    *ptr++ = akku;
    }
    while( akku != '\0' );
  return buffer;
  }

/*
**  Ausgeben einer Zeile
**    data : Zeiger auf die auszugebende Zeile
*/
static void LOCAL auszeile ( zeile* data )
  {
  const char* str = data->data;
  str = chars2tex( str );
  str = zahlen2tex( str );
  outstring( str );
  }

/*
**  Durchführen des Plottens
**    n : Anzahl der Werte
**    x : die Werte
*/
static void LOCAL plot ( int n, const unsigned char* x )
  {
  unsigned char lasty = *x;
  const int nm = n - 1;
  int i;
  for( i = 0; i < nm; ++i )
    {
    unsigned char y = x[i];
    if( y > lasty )
      out_printf( "\\TG{%d}{%d}{%d} ", i, y, y - lasty );
    else if ( y < lasty )
      out_printf( "\\TH{%d}{%d}{%d} ", i, y, lasty - y );
    else
      out_printf( "\\put(%d,%d){\line(1,0){1} ", i, y );
    lasty = y;
    if( ( i & 7 ) == 7 ) out_puts( "\r\n" );
    }
  if( x[nm] > lasty )
    out_printf( "\\put(%d,%d){\\line(-1,0){%d}\r\n", nm, x[nm], x[nm] - lasty );
  else
    out_printf( "\\put(%d,%d){\\line(1,0){%d}\r\n", nm, x[nm], lasty - x[nm] );
  }

/*
**  Ausgeben eines Graphen
**    data : Zeiger auf den Graphen
*/
static void LOCAL ausgraf ( graf* data )
  {
  const char* headline = getstring();
  out_puts( "\\begin{figure}[htb]\r\n" );
  if( headline )
    {
    out_printf( "\\capital{%s}\r\n", headline );
    free( headline );
    }
  out_puts( "\\begin{picture}(1008,256)\r\n" );
  out_puts( "\\usebox{\\TGKoordinaten}\r\n" );
  if( data->count != 1008 ) warning( "Graphen mit != 1008 Punkten.\n" );
  plot( data->count < 1008 ? data->count : 1008, data->data );
  out_puts( "\\end{picture}\r\n"
            "\\end{figure}\r\n"  );
  }

/*
**  Reaktion auf eine FF
**    data : Zeiger auf die ff-Struktur
*/
static void LOCAL ausff ( ff* data )
  {
  UNUSED( data );
  clearstring();
  out_puts( "%\r\n%\r\n" );
  }

/*
**  M A I N
*/
int main ( int argc, char* argv [] )
  {
  enum tgp typ;
  if( argc != 3 )
    {
    fputs( manual, stderr );
    exit( 1 );
    }
  plotfile_open( argv[1] );
  out_open( argv[2] );
  while( ( typ = plotfile_next() ) != _tgp_eof )
    {
    switch( typ )
      {
      case _tgp_zeile : auszeile( plotfile_getzeile() );
                        break;
      case _tgp_graf  : ausgraf( plotfile_getgraf() );
                        break;
      case _tgp_ff    : ausff( plotfile_getff() );
                        break;
      }
    }
  clearstring();
  out_close();
  plotfile_close();
  return 0;
  }

