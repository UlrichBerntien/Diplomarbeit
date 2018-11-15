/*
**  C   ( Microsoft C V5.10 )
**
**  .03.1993  Ulrich Berntien
**
**  Graphen auf Monitor ausgeben
**  und spezielle Textausgaben
**  benutzt die Microsoft Quick-C Grafikbibliothek
**
**  20.03.1993  Beginn
**  30.03.1993  Modul phase03.c aus phase1.c abgespalten
**  18.08.1993  Längen der Daten variabel
*/

#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dos.h>
#include <graph.h>
#include "phase.h"

/*
**  Tastencodes für Funktionstaste, Cursor Up und Cursor Down
*/
#define F_KEY  0x00
#define F_UP   0x48
#define F_DOWN 0x50

/*
**  Zeilen/Spalten im Grafikmodus verfügbar
*/
static int zeilen;
static int spalten;

/*
**  Zeichnet Koordinatensystem X,Y - Achse
**    Y0 : untere Grafikzeile
**    Y1 : obere Grafikzeile
**    x  : rechtes Ende
*/
static void LOCAL achsen ( unsigned Y0, unsigned Y1, unsigned x )
  {
  unsigned i;
  _moveto( 0, Y0 );
  _lineto( 0, Y1 );
  _lineto( x, Y1 );
  for( i = 0; i <= x; i += 100 )
    {
    _moveto( i, Y1 - 5 );
    _lineto( i, Y1 );
    }
  }

/*
**  Bestimmt obere und untere Grafikzeile für eine Bildschirmhälfte
**    oben  : 0 -> untere Hälfte, sonst obere Hälfte
**    Y0,Y1 : obere und untere Grafikzeile dorthin schreiben
*/
static void LOCAL getzeilen ( int oben, int* Y0, int* Y1 )
  {
  *Y0 = oben ? 0              : zeilen / 2;
  *Y1 = oben ? zeilen / 2 - 1 : zeilen - 1;
  }

/*
**  Monitor in Grafikmodus schalten
**  setzt globale Variablen 'spalten' und 'zeilen'
**    return : != 0, wenn Fehler aufgetreten
*/
int _cdecl grafik_open ( void )
  {
  const int error = _setvideomode( _VRES2COLOR ) == 0;
  if( error )
    {
    puts( "Grafikausgabe nur mit VGA-Karte" );
    _setvideomode( _DEFAULTMODE );
    }
  else
    {
    struct videoconfig cfg;
    _getvideoconfig( &cfg );
    spalten = cfg.numxpixels;
    zeilen = cfg.numypixels;
    }
  return error;
  }

/*
**  Monitor in ursprünglichen Modus schalten
*/
void _cdecl grafik_close ( void )
  {
  _setvideomode( _DEFAULTMODE );
  }

/*
**  Monitor löschen (Grafik oder Textmodus)
*/
void _cdecl grafik_cls ( void )
  {
  _clearscreen( _GCLEARSCREEN );
  }

/*
**  Teil des Monitors im Grafikmodus löschen und Cursor Home
**    oben : 0 -> Graph im unteren Bildschirmteil löschen, sonst oberen Teil
*/
void _cdecl grafik_clear ( int oben )
  {
  int Y0, Y1;
  getzeilen( oben, &Y0, &Y1 );
  _setviewport( 1, Y0, spalten-1, Y1-1 );
  _clearscreen( _GVIEWPORT );
  _setviewport( 0,0, spalten, zeilen );
  _settextposition( 1,1 );
  }

/*
**  String ausgeben
**    x,y : Startposition des Strings
**          falls x < 0, dann aktuelle Textcursor-Position benutzten
**    str : der auszugebende String
*/
void _cdecl grafik_puts( int x, int y, const char* str )
  {
  if( x >= 0 ) _settextposition( y,x );
  _outtext( (char _far*) str );
  }

/*
**  Ausgabe einer uchar - Tabelle als Grafik auf Monitor
**    oben : 1 -> obere Bildschirmhälfte, 0 -> untere Hälfte
**    x    : der Graph
*/
void _cdecl grafik_grafuc ( int oben, const grafuc* x )
  {
  const int n = x->len > spalten ? spalten : x->len;
  int Y0,Y1;
  unsigned i,delta;
  getzeilen( oben, &Y0, &Y1 );
  delta = Y1 - Y0;
  achsen( Y0, Y1, n-1 );
  _moveto( 0, Y1 - ( (unsigned) x->data[0] * delta ) / 255 );
  for( i = 1; i < n; ++i )
    _lineto( i, Y1 - ( (unsigned) x->data[i] * delta ) / 255 );
  }

/*
**  Ausgabe einer long - Tabelle als Grafik auf Monitor
**    oben : 1 -> obere Bildschirmhälfte, 0 -> untere Bildschirmhälfte
**    x    : der Graph
*/
void _cdecl grafik_grafl ( int oben, const grafl* x )
  {
  const int n = x->len > spalten ? spalten : x->len;
  int i,delta;
  int Y0,Y1;
  long divisor;
  getzeilen( oben, &Y0, &Y1 );
  delta = Y1 - Y0;
  divisor = ( x->max - x->min + delta-1 ) / delta;
  if( divisor == 0 ) divisor = 1;
  delta = (int) ( x->min / divisor );
  achsen( Y0, Y1, n-1 );
  _moveto( 0, Y1 - (int) ( x->data[0] / divisor ) + delta );
  for( i = 1; i < n; ++i )
    _lineto( i, Y1 - (int) ( x->data[i] / divisor ) + delta );
  }

/*
**  String zentriert ausgeben
**    zeile : in diese Zeile
**    str   : der String
*/
static void LOCAL zentriert ( int zeile, const char* str )
  {
  const int len = strlen( str );
  grafik_puts( (80-len)/2, zeile, str );
  }

/*
**  Textposition auf Position (x,y) unter der Überschrift
*/
void _cdecl grafik_home( int x, int y )
  {
  _settextposition( y + 4 , x + 1 );
  }

/*
**  CLS und Überschrift ausgeben
**    name : Name des Programms
*/
void _cdecl grafik_headline ( const char* name )
  {
  static const char* N1 = "Institut für Experimentalphysik - Fokusgruppe";
  _clearscreen( _GCLEARSCREEN );
  zentriert( 1, N1 );
  zentriert( 2, name );
  grafik_home( 0,0 );
  }

/*
**  Cursorposition über BIOS - Aufruf setzen
**    x,y : Position des Cursors (0,0) = links oben
*/
static void LOCAL setcursor( int x, int y )
  {
  union REGS regs;
  regs.h.bh = (uchar) 0;
  regs.h.dh = (uchar) y;
  regs.h.dl = (uchar) x;
  regs.h.ah = (uchar) 2;
  int86( 0x10, &regs, &regs );
  }

/*
**  Menü anzeigen und Auswahl ermöglichen
**    liste  : Liste  von Zeigern auf Strings, durch NULL beendet
**    selekt : auf diesem Menüpunkt Cursor setzen
**    return : Nummer des gewählten Punktes 0,..
*/
int _cdecl grafik_menu ( const char** liste, int selekt )
  {
  const char** ptr = liste;
  const int x = 20;
  const int y = 6;
  int key;
  int count = 0;
  while( *ptr != NULL ) grafik_puts( x+2, y + count++, *ptr++ );
  if( selekt < 0 || selekt >= count ) selekt = 0;
  do{
    grafik_puts( x, y+selekt, "\x10" );
    setcursor( 0, 25 );
    do{
      key = getch();
      if( key == F_KEY )
        {
        key = getch();
        if( key != F_UP && key != F_DOWN ) key = 0;
        }
      else
        if( key != '\r' ) key = 0;
      }
      while( ! key );
    if( key != '\r' )
      {
      grafik_puts( x, y+selekt, " " );
      selekt += key == F_DOWN ? 1 : count - 1;
      selekt %= count;
      }
    }
    while( key != '\r' );
  _settextposition( 22,1 );
  return selekt;
  }

