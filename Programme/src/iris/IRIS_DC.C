/*
**  C   ( Zortech C V2.1  /  Micrsoft C V5.10 / Turbo C V2.0 )
**
**  .04.1993  Ulrich Berntien
**
**  Bild von Kamera kontinuierlich anzeigen und Cursor einblenden
**
**  14.04.1993  Beginn
*/

#include <conio.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _far
#  define _far    far
#  define _near   near
#  define _cdecl  cdecl
#  define _pascal pascal
#endif

#include "isdefs.h"

#define LOCAL _near _pascal
#define UNUSED(x) (void)(x)

/*
**  Kurzbeschreibung des Programms
*/
static const char manual [] =
  "IRIS Display + Cursor   .04.1993 Ulrich Berntien\n"
  "Benutzt die Bildverarbeitungskarte DT-IRIS.\n"
  "Das Bild vom Kanal 1 wird direkt ausgegeben (pass thru)\n"
  "und ein Fadenkreuz eingeblendet.\n\n"
  "Aufrufformat:\n"
  "               IRIS_DC\n";

/*
**  Anleitung zur Bewegung des Cursor
*/
static const char anleitung [] =
  "\nCursor bewegen mit den Zifferntasten\n"
  "                 8     \n"
  "            4         6\n"
  "                 2     \n"
  "Geschwindigkeit        Programmende\n"
  "  7 schneller                e\n"
  "  1 langsamer\n";

/*
**  Falls Fehler von DT-IRIS-Treiber gemeldet wurde
**  Programm abbrechen
*/
static void LOCAL error ( void )
  {
  fputs( "Fehler vom DT-IRIS Treiber gemeldet, Programmabbruch\n", stderr );
  is_abort();
  exit( 1 );
  }

/*
**  Karte initialisieren
*/
static void LOCAL init ( void )
  {
  if( is_initialize() || is_reset() ) error();
  }

/*
**  Karte abschalten
*/
static void LOCAL termit ( void )
  {
  if( is_reset() || is_end() ) error();
  }

/*
**  Bild von Eingabekanal 1 direkt ausgeben
*/
static void LOCAL display ( void )
  {
  if( is_select_output_frame( 0 ) ||
      is_select_input_frame( 0 )  ||
      is_select_channel( 1 )      ||
      is_display( 1 )             ||
      is_set_sync_source( 1 )     ||
      is_passthru()                ) error();
  }

/*
**  Cursor anzeigen und bewegen
*/
static void LOCAL cursor ( void )
  {
  int zeile = 255;
  int spalte = 255;
  int speed = 2;
  int wahl;
  if( is_cursor( 1 ) ) error();
  puts( anleitung );
  do{
    if( is_set_cursor_position( zeile, spalte ) ) error();
    printf( "   Zeile = %3d   Spalte = %3d\r", zeile, spalte );
    wahl = getch();
    switch( wahl )
      {
      case '4' : if( spalte - speed >= 0 ) spalte -= speed;
                 break;
      case '6' : if( spalte + speed < 512 ) spalte += speed;
                 break;
      case '8' : if( zeile - speed >= 0 ) zeile -= speed;
                 break;
      case '2' : if( zeile + speed < 512 ) zeile += speed;
                 break;
      case '1' : if( speed > 1 ) speed /= 2;
                 break;
      case '7' : if( speed < 128 ) speed *= 2;
                 break;
      }
    }
    while( wahl != 'e' && wahl != 'E' );
  if( is_cursor( 0 ) ) error();
  puts( "\nPrgrammende." );
  }

/*
**  M A I N
*/
int main ( int argc, char* argv [] )
  {
  UNUSED( argv );
  if( argc != 1 )
    fputs( manual, stderr );
  else
    {
    init();
    display();
    cursor();
    termit();
    }
  return 0;
  }

