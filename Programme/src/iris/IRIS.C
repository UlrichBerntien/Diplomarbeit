/*
**  C  ( Zortech C V2.1 / Microsoft C V5.1 / Turbo C V2.0 )
**
**  Ulrich Berntien .08.1993
**
**  23.08.1993  Beginn (ersetzt das Progamm IS30.C von den Autoren
**              D.Meiners, H.Wenz und J.Westheide)
**  14.09.1993  Funktion "bildspiegeln" im Menü "verarbeiten" aufgenommen
**  25.07.1994  Funktion "loadvollNONIRIS" im Menü "File <-> Bild"
**              aufgenommen
**  29.07.1994  Menü "sonstige Parameter" aufgenommen
**  02.09.1994  Funktion "drawstr_background" aufgenommen
**  19.12.1994  Funktionen "luteditor" und "saveSplitterDialog" aufgenommen
*/

#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "iris.h"

/*
**  Tastaturcodes
*/
#define FKEY   0x0000
#define UP     0x0148
#define DOWN   0x0150
#define HOME   0x0147
#define END    0x014F
#define ESC    0x001B
#define ENTER  0x000D

#define UNUSED(x) ( (void)(x) )

/********************************************************************/

static const char* hauptmenu [] =
  {
  "aufnehmen",
  "1. Framegrabber",
  "2. Framegrabber",
  "3. Framegrabber",
  "beschriften, splitten, speichern",
  "verarbeiten",
  "Gould -> PC",
  "File <-> Bild",
  "DOS - Tools",
  "sonstige Parameter",
  NULL
  };

static const char* bildverarbeitung [] =
  {
  "Spektrum aus Bild gewinnen",
  "direkt Spektrum anzeigen",
  "n(x) und T(x) Grafik aus Bild gewinnen",
  "Helligkeitsverteilung über den Spalt",
  "Bild auf den Kopf stellen",
  "spiegeln des Bilds",
  "LUT optimieren",
  "Grauwerte eines Gebiets / Zoomen",
  NULL
  };

static const char* dostools [] =
  {
  "Kommando",
  "Command von DOS",
  "Volkov-Commander",
  "Directory zeigen",
  NULL
  };

static const char* gouldpc [] =
  {
  "Daten übertragen",
  "Namensvorgaben ändern",
  "Ein/Ausschalten der Automatik",
  NULL
  };

static const char* bildfile [] =
  {
  "lade Halbbild",
  "speicher Halbbild",
  "speicher abgesplittetes Halbbild",
  "Vollbild laden",
  "Lade Bild im DT-IRIS-FORMAT",
  "Speicher Bild im DT-IRIS-FORMAT",
  "Pfadname für Halbbild-Files",
  "Namen der Sicherungsfiles",
  NULL
  };

static const char* sonstige [] =
  {
  "Scans als Fläche oder nur Line",
  "Texthintergrund schwarz oder transparent",
  "Tabellenausgabe der wahren Werte bestimmen",
  NULL
  };

/*
**  Automatik, Nach Bildaufnahme Daten von Gould lesen
*/
static int automatik_gouldpc = 1;

/*
**  benutzter Framebuffer
*/
static int frb = 0;

/********************************************************************/

/*
**  String zentriert auf Bildschirm ausgeben
**  str : der String
**  ln  : die Bildschirmzeile 0..24
*/
static void LOCAL putszentral ( const char* str, int ln )
  {
  int y = ( 80 - strlen(str) ) / 2;
  if( y < 0 ) y= 0;
  putsxy( y, ln, str );
  }

/*
**  Bildschirm löschen und Überschrift bringen
**  name : des Programmabschnitts
*/
static void LOCAL headline ( const char* name )
  {
  cls();
  putsxy( 0,0, "IRIS .1994" );
  putsxy( 80 - 12 ,0, "Fokus-Gruppe" );
  putszentral( name, 1 );
  setcrrspos( 0, 3 );
  }

/*
**  Das Menü ausgeben
**  items  : Liste der Wahlmöglichkeiten, mit NULL terminiert
**  x,y    : Position des Menüs aus dem Bildschirm
**  return : Anazhl der Menüpunkte
*/
static int LOCAL menuausgabe ( const char** items, int x, int y )
  {
  int i = 0;
  putszentral( "CRRS UP/DOWN, 1.Zeichen und ENTER, ESC", 24 );
  while( items[i] != NULL )
    {
    putsxy( x, y+i, items[i] );
    ++i;
    }
  setcrrspos( 200, 200 );
  return i;
  }

/*
**  Einen Menüpunkt nach seinem ersten Buchstaben aussuchen
**  was    : danach suchen
**  items  : Liste der Menüpunkte, mit NULL terminiert
**  start  : Index, dort mit der Suche beginnen
**  return : ausgesuchter Menüpunkt
*/
static int LOCAL menusuche ( char was, const char** items, int start )
  {
  int i = start;
  while( items[i] != NULL && *items[i] != was ) ++i;
  if( items[i] == NULL )
    {
    i = 0;
    while( i < start && *items[i] != was ) ++i;
    }
  return i;
  }

/*
**  einen Menüeintrag markieren
**  x,y : die Position des Eintrags auf dem Bildschirm
*/
static void LOCAL menumark ( int x, int y )
  {
  putsxy( x-3, y, ">>" );
  }

/*
**  Markierung eines Menüeintrags löschen
**  x,y : die Position des Eintrags auf dem Bildschirm
*/
static void LOCAL menudelmark ( int x, int y )
  {
  putsxy( x-3, y, "  " );
  }

/*
**  Menü zur Auswahl präsentieren
**  items  : Liste der Wahlmöglichkeiten mit NULL terminiert
**  wahl   : diesen Menüpunkt zuerst markieren
**  x,y    : dort das Menü auf den Bildschirm bringen
**  return : Nummer des gewählten Menüpunkts,
**           oder -1 bei verlassen des Menüs mit ESC
*/
static int LOCAL menu ( const char** items, int wahl, int x, int y )
  {
  int key;
  const int count = menuausgabe( items, x, y );
  if( wahl < 0 || wahl >= count ) wahl = 0;
  menumark( x, y + wahl );
  clearkbd();
  do{
    int neuewahl = wahl;
    key = getch();
    if( key == FKEY ) key = getch() | 0x0100;
    switch( key )
      {
      case UP   : neuewahl = wahl < 1 ? count - 1 : wahl - 1;
                  break;
      case DOWN : neuewahl = wahl + 1 < count ? wahl + 1 : 0;
                  break;
      case HOME : neuewahl = 0;
                  break;
      case END  : neuewahl = count - 1;
                  break;
      default   : if( key < 0x100 && isalnum( key ) )
                    neuewahl = menusuche( (char) key, items, wahl );
      }
    if( wahl != neuewahl )
      {
      menudelmark( x, y + wahl );
      wahl = neuewahl;
      menumark( x, y + wahl );
      }
    }
    while( key != ENTER && key != ESC );
  clearkbd();
  return key == ESC ? -1 : wahl;
  }

/*
**  Falls ein nicht existierender Menüpunkt angewählt
*/
static void LOCAL menuerror ( void )
  {
  headline( "Fehler" );
  puts( "interner Fehler aufgetreten (menuerror)" );
  exit( 1 );
  }

/********************************************************************/

/*
**  Umschalten, des Automatik der Datenübertragung Gould -> PC
*/
static void LOCAL switchautomatik ( void )
  {
  automatik_gouldpc = ! automatik_gouldpc;
  putszentral( "Automatische Datenübertragung DSO Gould -> PC ist nun", 4 );
  putszentral( automatik_gouldpc ? "eingeschaltet" : "ausgeschaltet", 6 );
  setcrrspos( 37, 8 );
  getreturn();
  }

/*
**  Menü Gould <-> PC
**  name : Name des Menüs
*/
static void LOCAL menu_gouldpc ( const char* name )
  {
  static int wahl = 0;
  int iwahl;
  do{
    headline( name );
    iwahl = menu( gouldpc, wahl, 30, 10 );
    if( iwahl >= 0 ) headline( gouldpc[ wahl = iwahl ] );
    switch( iwahl )
      {
      case  0 : goulddaten( 1, 1 );
                break;
      case  1 : gouldnamen();
                break;
      case  2 : switchautomatik();
                break;
      case -1 : break;
      default : menuerror();
      }
    }
    while( iwahl >= 0 );
  }

/*
**  Menü Bild <-> File
**  name : Name des Menüs
*/
static void LOCAL menu_bildfile ( const char* name )
  {
  static int wahl = 0;
  int iwahl;
  do{
    headline( name );
    iwahl = menu( bildfile, wahl, 30, 6 );
    if( iwahl >= 0 ) headline( bildfile[ wahl = iwahl ] );
    switch( iwahl )
      {
      case  0 : loaddialog( frb );
                break;
      case  1 : savedialog( frb, NULL );
                break;
      case  2 : saveSplitterDialog( frb, NULL );
                break;
      case  3 : loadvollNONIRIS( frb );
                break;
      case  4 : loadvoll( frb );
                break;
      case  5 : savevoll( frb );
                break;
      case  6 : setbildpfad();
                break;
      case  7 : setsavefiles();
                break;
      case -1 : break;
      default : menuerror();
      }
    }
    while( iwahl >= 0 );
  }

/*
**  Menü DOS - Tools
**  name : Name des Menüs
*/
static void LOCAL menu_dostools ( const char* name )
  {
  static int wahl = 0;
  int iwahl;
  do{
    headline( name );
    iwahl = menu( dostools, wahl, 30, 10 );
    if( iwahl >= 0 ) headline( dostools[ wahl = iwahl ] );
    switch( iwahl )
      {
      case  0 : doscmd();
                break;
      case  1 : dosshell(0);
                break;
      case  2 : dosshell(1);
                break;
      case  3 : dosdir();
                break;
      case -1 : break;
      default : menuerror();
      }
    }
    while( iwahl >= 0 );
  }

/*
**  Menü Bildverarbeitung
**  name : Name des Menüs
*/
static void LOCAL menu_bildverarbeitung ( const char* name )
  {
  static int wahl = 0;
  int iwahl;
  do{
    headline( name );
    iwahl = menu( bildverarbeitung, wahl, 30, 6 );
    if( iwahl >= 0 ) headline( bildverarbeitung[ wahl = iwahl ] );
    switch( iwahl )
      {
      case  0 : spektren( frb );
                break;
      case  1 : spektrenplus( frb );
                break;
      case  2 : nt_grafik( frb );
                break;
      case  3 : helligkeit( frb );
                break;
      case  4 : kopfstand( frb );
                break;
      case  5 : bildspiegeln( frb );
                break;
      case  6 : luteditor ( frb );
                break;
      case  7 : displaygrauwerte( frb );
                break;
      case -1 : break;
      default : menuerror();
      }
    }
    while( iwahl >= 0 );
  }

/*
**  Menü sonstige Parameter
**  name : Name des Menüs
*/
static void LOCAL menu_sonstige ( const char* name )
  {
  static int wahl = 0;
  int iwahl;
  do{
    headline( name );
    iwahl = menu( sonstige, wahl, 20, 10 );
    if( iwahl >= 0 ) headline( sonstige[ wahl = iwahl ] );
    switch( iwahl )
      {
      case  0 : changescan();
                break;
      case  1 : drawstr_background();
                break;
      case  2 : wwtabelle();
                break;
      case -1 : break;
      default : menuerror();
      }
    }
    while( iwahl >= 0 );
  }

/*
**  Nach Aufnahme der Bilder
**   . Bilder in die Sicherungsfiles
**   . Daten von dem DSO Gould lesen
*/
static void LOCAL nachtrigger ( void )
  {
  putchar( '\a' );
  savefiles( frb );
  if( automatik_gouldpc ) goulddaten( 0,0 );
  }

/*
**  Hauptmenü
*/
static void LOCAL menu_haupt ( void )
  {
  int wahl = 0;
  do{
    headline( "Hauptmenü" );
    wahl = menu( hauptmenu, wahl, 30, 6 );
    if( wahl >= 0 ) headline( hauptmenu[wahl] );
    switch( wahl )
      {
      case  0 : if( frg_aufnehmen( frb ) ) nachtrigger();
                break;
      case  1 :
      case  2 :
      case  3 : frg_framegrabber( wahl );
                break;
      case  4 : fastpart( frb );
                break;
      case  5 : menu_bildverarbeitung( hauptmenu[wahl] );
                break;
      case  6 : menu_gouldpc( hauptmenu[wahl] );
                break;
      case  7 : menu_bildfile( hauptmenu[wahl] );
                break;
      case  8 : menu_dostools( hauptmenu[wahl] );
                break;
      case  9 : menu_sonstige( hauptmenu[wahl] );
                break;
      case -1 : headline( "Programmende" );
                fputs( "Programm beenden ?", stdout );
                break;
      default : menuerror();
      }
    }
    while( wahl >= 0 || ! janein() );
  }

/*
**  M A I N
*/
int main ( int argc, char* argv [] )
  {
  UNUSED( argc );
  UNUSED( argv );
  headline( "Initialisierung" );
  frg_init( NULL );
  menu_haupt();
  frg_termit();
  return 0;
  }
