/*
**  C   ( Zortech C V2.1 / Microsoft C C5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .10.1992
**
**  WARNUNG : benötigt bis zu 2 KByte auf dem Stack
**
**  Spektren erstellen
**
**  03.10.1992  Beginn
**  08.10.1992  Verknüpfung mit 'iris04.c'
**  28.10.1992  Fehler in calcspektrum() beseitigt, 'count' war 1 zu groß
**  03.11.1992  Auswertung in get_display() ermöglicht
**  28.07.1993  Funktion stredit() benutzt
**  08.07.1994  Funktion spwahrewerte() benutzt
**  29.07.1994  Funktion changescan() eingeführt
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iris.h"
#include "common.h"

/********************************************************************/

TEXT crrstext [] =
  "Bewegen des Zeilencursors:\n"
  "8 - höher , 2 - tiefer, 5 -stop,\n"
  "f - schneller, s - langsamer.";

TEXT crrswahl [] = "852sf";

TEXT obere  [] = "<> obere Grenze ";
TEXT untere [] = "<> untere Grenze ";

TEXT head [] =
  "Das Bild muß aus zwei gleichen Halbbildern bestehen.\n"
  "Das Bild wird später durch die Spektren überschrieben !";

/*
**  Soll in den Scans die Kurve gefüllt werden ?
**  (sonst nur eine Linie)
*/
static int fuellen = 1;

/********************************************************************/

/*
**  Umschalten der Darstellung der Scans von "nur Linie" auf
**  "Fläche unterhalb der Linie".
*/
void changescan ( void )
  {
  fuellen = ! fuellen;
  putsxy( 20, 8, "Die Scans werden nun in der Darstellung" );
  putsxy( 20,10, fuellen ? "Fläche unterhalb der Kurve"
                      : "nur Linie" );
  putsxy( 20,12, "ausgegeben." );
  setcrrspos( 20, 14 );
  getreturn();
  }

/*
**  Löscht Zeilen im Framebuffer ( Grauwert auf 0 setzen )
**  frb : Framebuffer Nummer
**  von : erste zu löschende Zeile (0..zbp)
**  bis : letzte zu löschende Zeile
*/
static void LOCAL clear ( int frb, int von, int bis )
  {
  char leer [bpz];
  memset( leer, 0, sizeof leer );
  while( von <= bis )
    isb_put_pixel( frb, von++, wpz, leer );
  }

/*
**  Schreibt die Angabe "von - bis" ins Bild
**  frb   : Framebuffer Nummer
**  zeile : Unterhalb dieser Bildzeile rechtsbüdig schreiben
**  sp    : Von diesem Spektrum die Zeilenangeben schreiben
*/
static void LOCAL zeilenschrift ( int frb, int zeile, const spektrum* sp )
  {
  char buffer [20];
  sprintf( buffer, "%d - %d", sp->von, sp->bis );
  is_draw_str( frb, zeile+4, 1, buffer );
  }

/*
**  Gibt ein Spektrum in das Bild, überschreibt dabei eine Bildhälfte
**  frb   : Framebuffer Nummer
**  zeile : ab dieser Zeile das zpb/2 Zeilen benutzten für die Darstellung
**  sp    : das darzustellende Spektrum
*/
void spektrumout( int frb, const int zeile, const spektrum* sp )
  {
  char buffer [bpz];
  const int* wert = sp->wert;
  int znow = zeile;
  int i;
  memset( buffer, dunkel, sizeof buffer );
  buffer[0] = hell;
  if( fuellen )
    for( i = 0xff; i >= 0; --i )
      {
      int j;
      for( j = 1; j < bpz; ++j )
        if( wert[j] == i ) buffer[j] = hell;
      isb_put_pixel( frb, znow++, wpz, buffer );
      }
  else
    for( i = 0xff; i >= 0; --i )
      {
      int j;
      for( j = 1; j < bpz; ++j )
        if( wert[j-1] < wert[j] )
        {
        if( wert[j] == i ) buffer[j] = hell;
        else if( wert[j-1] >= i ) buffer[j] = dunkel;
        }
      else
        {
        if( wert[j-1] == i ) buffer[j] = hell;
        else if( wert[j] >= i ) buffer[j] = dunkel;
        }
    isb_put_pixel( frb, znow++, wpz, buffer );
    }
  zeilenschrift( frb, zeile, sp );
  }

/*
**  Anzeige der Spektren, jeweils zwei Spektren in ein Bild
**  frb  : Framebuffer Nummer
**  sp   : Beginn der einfachverketteten Liste mit den Spektren
**  name : Name des Spektren
*/
static void LOCAL display( int frb, spektrum* sp, const char* name )
  {
  int ausgabe;
  int auswertung;
  if( sp != NULL )
    {
    fputs( "Spektren ausgeben" , stdout );
    ausgabe = janein();
    if( ausgabe )
      {
      fputs( "Auswertung der Spektren", stdout );
      auswertung = janein();
      }
    }
  while( sp != NULL && ausgabe )
    {
    printf( "oben  Zeilen %3d - %3d\n", sp->von, sp->bis );
    spektrumout( frb, 0, sp );
    if( auswertung )
      {
      clear( frb, zpb/2, zpb-1 );
      spauswertung( 0, sp, frb, name );
      }
    sp = sp->next;
    if( sp != NULL )
      {
      printf( "unten Zeilen %3d - %3d\n", sp->von, sp->bis );
      spektrumout( frb, zpb/2, sp );
      if( auswertung ) spauswertung( zpb/2, sp, frb, name );
      sp = sp->next;
      }
    else
      clear( frb, zpb/2, zpb-1 );
    if( name[0] ) is_draw_str( frb, 4, 0, name );
    if( sp != NULL )
      {
      fputs( "Nächste Spektren ausgeben", stdout );
      ausgabe = janein();
      }
    }
  }

/********************************************************************/

/*
**  Neues Spektrum anlegen, setzt alle Felder auf Startwerte
**  von    : ab dieser Zeile wurde gemittelt
**  bis    : bis zu dieser Zeile wurde gemittelt
**  return : Zeiger auf neuangelegten Speicherbereich, oder NULL
*/
static spektrum* LOCAL neuesspektrum ( int von, int bis )
  {
  spektrum *const sp = malloc( sizeof (spektrum) );
  if( sp != NULL )
    {
    if( von < bis )
      {
      sp->von = von;
      sp->bis = bis;
      }
    else
     {
     sp->von = bis;
     sp->bis = von;
     }
    sp->next = NULL;
    memset( sp->wert, 0, sizeof sp->wert );
    }
  return sp;
  }

/*
**  Spektum aus Bild gewinnen
**  (nur jede zweite Zeile genommen, weil Halbbilder gleich sind)
**  frb    : Framebuffer Nummer
**  von    : erste Zeile ab der gemittelt wird
**  bis    : letzte Zeile bis zu der gemittelt wird
**    (die Zeilengrenzen werden geordnet)
**  return : das gemittelte Spektrum, oder NULL im Fehlerfall
*/
static spektrum* LOCAL read ( int frb, int von, int bis )
  {
  spektrum *const sp = neuesspektrum( von, bis );
  if( sp != NULL )
    {
    uchar buffer [bpz];
    int* wert = sp->wert;
    int zeilen = 0;
    int i;
    von = sp->von;
    bis = sp->bis;
    while ( von <= bis )
      {
      isb_get_pixel( frb, von, wpz, (char*) buffer );
      von += 2;
      for( i = 0; i < bpz; ++i ) wert[i] += buffer[i];
      ++zeilen;
      }
    for( i = 0; i < bpz; ++i ) wert[i] /= zeilen;
    }
  return sp;
  }

/*
**  Bestimmt Zeile im Dialog mit dem Benutzer.
**  Zeile aus dem gesamten Bildbereich 0..zpb, aber nur jede zweite
**  Zeile kann angewählt werden.
**  frb    : Framebuffer Nummer
**  zeile  : Startzeile 0..zpb-1
**  return : Ausgewählte Zeile
*/
int zeilencrrs ( int frb, int zeile )
  {
  int buffer [2*wpz];
  int negiert [2*wpz];
  char wahl;
  int i;
  int speed = 2;
  do{
    isb_get_pixel( frb, zeile, 2*wpz, (char*) buffer );
    for( i = 0; i < 2*wpz; ++i ) negiert[i] = ~buffer[i];
    isb_put_pixel( frb, zeile, 2*wpz, (char*) negiert );
    wahl = askcset( crrswahl );
    isb_put_pixel( frb, zeile, 2*wpz, (char*) buffer );
    switch( wahl )
      {
      case '2' : if( zeile + speed < zpb ) zeile += speed;
                 break;
      case '8' : if( zeile - speed >= 0 ) zeile -= speed;
                 break;
      case 's' : if( speed > 2 ) speed /= 2;
                 break;
      case 'f' : if( speed < 500 ) speed *= 2;
      }
    }
    while( wahl != '5' );
  return zeile;
  }

/*
**  Bestimmt Anfangs- und Endzeile des Bereichs für die Erfassung des Spektrums
**  frb : Framebuffer Nummer
**  von : Zeiger auf Startwert / Rückgabe der ausgewählten Startzeile
**  bis : Rückgabe der ausgewählten Endzeile
**   (von, bis sind nicht sortiert, d.h. bis < von möglich)
*/
static void LOCAL vonbis ( int frb, int* von, int* bis )
  {
  char buffer [zpb];
  char black [zpb];
  memset( black, 0xff, sizeof black );
  fputs( obere, stdout );
  *von = zeilencrrs( frb, *von );
  isb_get_pixel( frb, *von, wpz, buffer );
  isb_put_pixel( frb, *von, wpz, black );
  printf( "%3d\n%s", *von, untere );
  *bis = zeilencrrs( frb, *von );
  isb_put_pixel( frb, *von, wpz, buffer );
  printf( "%3d\n", *bis );
  }

/*
**  Erfassen der Spektren im Benutzerdialog
**  frb    : Framebuffer Nummer
**  return : Anfang auf die einfachverketteten Liste der erfassten Spektren
*/
static spektrum* LOCAL get ( int frb )
  {
  spektrum* root = NULL;
  spektrum* now = NULL;
  int von = zpb/2;
  int bis = zpb/2;
  char wahl;
  puts( crrstext );
  do{
    spektrum* sp;
    do{
      vonbis( frb, &von, &bis );
      puts( "n - nächster Bereich\n"
            "w - Wiederholung diese Festlegung\n"
            "f - fertig" );
      wahl = getcset( "nfw" );
      }
      while( wahl == 'w' );
    sp = read( frb, von, bis );
    if( sp == NULL )
      {
      errmalloc();
      break;
      }
    if( root != NULL )
      now = now->next = sp;
    else
      now = root = sp;
    }
    while( wahl != 'f' );
  return root;
  }

/*
**  Löscht Liste mit den Spektren
**  sp : Zeiger auf den Anfang der einfachverketteten Liste der Spektren
*/
static void LOCAL loesche ( spektrum* sp )
  {
  while( sp != NULL )
    {
    spektrum *const help = sp->next;
    free( sp );
    sp = help;
    }
  }

/*
**  Name der Spektren einlesen, Buffer für den Namen wird hier zur
**  Verfügung gestellt
**  return : Zeiger auf den Namen
*/
static const char* LOCAL getname ( void )
  {
  static char name [50];
  fputs( "Name der Spektren : ", stdout );
  stredit( name, sizeof name );
  return name;
  }

/*
**  Liste der Spektren ausgeben und löschen
**  frb  : Framebuffer Nummer
**  root : Zeiger auf den Anfang der verkettenen Liste mit den Spektren
**  name : der Name der Spektren
*/
static void LOCAL outall ( int frb, spektrum* root, const char* name  )
  {
  display( frb, root, name );
  loesche( root );
  }

/********************************************************************/

/*
**  Bild aus zwei gleichen Halbbildern wird in obere Bildhälfte geschoben
**  frb : Framebuffer Nummer
*/
void press ( int frb )
  {
  char buffer [bpz];
  int source = 0;
  int destination = 0;
  while( source < zpb )
    {
    isb_get_pixel( frb, source += 2, wpz, buffer );
    isb_put_pixel( frb, ++destination, wpz, buffer );
    }
  }

/*
**  Zeilennummer der Spektren anpassen, von Halbbild 0..255 auf
**  Gesamtbild 0..511
**  sp : Zeiger auf Beginn der einfachverketten Liste der Spektren
*/
static void LOCAL adjust ( spektrum* sp )
  {
  while( sp )
    {
    sp->von *= 2;
    sp->bis *= 2;
    sp = sp->next;
    }
  }

/*
**  Zeilencursor einblenden, vorher die Zeile sichern
**  Die Zeile 'sperre' darf nicht verändert werden, ihr Inhalt ist in
**  'sb' gespeichert
**  frb    : Framebuffer Nummer
**  zeile  : in diese Zeile Cursor einblenden
**  buffer : hier die überblendete Zeile sichern
**  sperre : diese Zeile ist auf dem Bildschirm nicht aktuell
**  sb     : Inhalt der Zeile 'sperre'
*/
static void LOCAL setcrrs ( int frb, int zeile, char buffer[bpz], int sperre, char sb[bpz] )
  {
  char marked [bpz];
  int i;
  if( zeile != sperre )
    {
    isb_get_pixel( frb, zeile, wpz, buffer );
    for( i = 0; i < bpz; ++i )
      marked[i] = (char) 0x80 ^ buffer[i];
    isb_put_pixel( frb, zeile, wpz, marked );
    }
  else
    memcpy( buffer, sb, bpz );
  }

/*
**  Bewegen des Cursors im oberen Bildhälfte 0..zpb/2
**  frb    : Framebuffer Nummer
**  pzeile : Startzeile / Rückgabe der Cursor-Zeilennummer
**  buffer : dort die von Cursor überschrieben Zeile sichern
**  sperre : diese Zeile ist auf dem Bildschirm nicht aktuell
**  sb     : das ist der Inhalt der Zeile 'sperre'
*/
static void LOCAL movecrrs ( int frb, int* pzeile, char buffer[bpz], int sperre, char sb[bpz] )
  {
  int zeile = *pzeile;
  int speed = 1;
  char wahl;
  do{
    wahl = askcset( crrswahl );
    if( zeile != sperre )
      isb_put_pixel( frb, zeile, wpz, buffer );
    switch( wahl )
      {
      case '8' : if( zeile - speed >= 0 ) zeile -= speed;
                 break;
      case '2' : if( zeile + speed < zpb/2 ) zeile += speed;
                 break;
      case 's' : if( speed > 1 ) speed /= 2;
                 break;
      case 'f' : if( speed < 500 ) speed *= 2;
      }
    setcrrs( frb, zeile, buffer, sperre, sb );
    }
    while( wahl != '5' );
  *pzeile = zeile;
  }

/*
**  Eine Zeile im Buffer zum Spektrum addieren
**  wert   : In diesem Feld werden die Zeilen addiert
**  buffer : die Zeile aus diesem Buffer aufaddieren
*/
static void LOCAL addspektrum ( int* wert, const char buffer[bpz] )
  {
  int i;
  for( i = 0; i < bpz; ++i ) wert[i] += (uchar) buffer[i];
  }

/*
**  Liest Zeilen zwischen 'sp->von' und 'sp->bis' vom Bildschirm und
**  führt den Mittellungsprozess durch, berücksichtigt dabei, daß schon
**  zwei Zeilen im Spektrum addiert sind
**  frb : Framebuffer Nummer
**  sp  : Zeiger auf die vorbereitete Datenstruktur zur Aufnahme des Spektrums
*/
static void LOCAL calcspektrum ( int frb, spektrum* sp )
  {
  uchar buffer [bpz];
  int* wert = sp->wert;
  int count = 0;
  int zeile = sp->von + 1;
  int i;
  while( zeile < sp->bis )
    {
    isb_get_pixel( frb, zeile, wpz, (char*) buffer );
    for( i = 0; i < bpz; ++i ) wert[i] += buffer[i];
    ++zeile;
    }
  count = sp->bis - sp->von + 1;
  for( i = 0; i < bpz; ++i ) wert[i] /= count;
  }

/*
**  Im Benutzerdialog Liste der Spektren zusammenstellen
**  gibt dabei das aktuelle Spektrum in der unteren Bildschirmhälfte aus
**  frb    : Framebuffer Nummer
**  name   : Name der Spektren
**  return : Zeiger auf Anfang der einfachverketten Liste der Spektren
*/
static spektrum* LOCAL get_display ( int frb, const char* name )
  {
  spektrum* root = NULL;
  spektrum* now = NULL;
  spektrum* sp;
  int von = zpb/4;
  int bis = zpb/4;
  char bufferv [bpz];
  char bufferb [bpz];
  char wahl;
  puts( crrstext );
  do{
    setcrrs( frb, bis, bufferb, -1, bufferv );
    setcrrs( frb, von, bufferv, bis, bufferb );
    do{
      puts( obere );
      movecrrs( frb, &von, bufferv, bis, bufferb );
      puts( untere );
      movecrrs( frb, &bis, bufferb, von, bufferv );
      sp = neuesspektrum( von, bis );
      if( sp != NULL )
        {
        addspektrum( sp->wert, bufferv );
        addspektrum( sp->wert, bufferb );
        calcspektrum( frb, sp );
        spektrumout( frb, zpb/2, sp );
        }
      else
        {
        errmalloc();
        break;
        }
      do{
        puts( "n - nächster Bereich festlegen\n"
              "w - Wiederholung dieser Festlegung\n"
              "a - Auswertung\n"
              "g - Spektrum aus Grauwerte in wahre Werte umrechnen\n"
              "f - fertig"                             );
        wahl = getcset( "nwagf" );
        if( wahl == 'a' ) spauswertung( zpb/2, sp, frb, name );
        if( wahl == 'g' ) spwahrewerte( zpb/2, sp, frb );
        }
        while( wahl == 'a' );
      }
      while( wahl == 'w' || wahl == 'g' );
    if( root != NULL )
      now = now->next = sp;
    else
      root = now = sp;
    if( sp == NULL ) break;
    isb_put_pixel( frb, bis, wpz, bufferb );
    isb_put_pixel( frb, von, wpz, bufferv );
    }
    while( wahl != 'f' );
  return root;
  }

/********************************************************************/

/*
**  Spektren aus einem Bild machen
**  frb : Framebuffer Nummer
*/
void spektren ( int frb )
  {
  spektrum* root;
  puts( head );
  root = get ( frb );
  if( root ) outall( frb, root, getname() );
  }

/*
**  Spektren aus einem Bild machen mit Anzeige der Spektren
**  unmittelbar nach Auswahl des Bereiches in der unteren
**  Bildschirmhälfte
**  frb : Framebuffer Nummer
*/
void spektrenplus ( int frb )
  {
  puts( head );
  fputs( "O.k.", stdout );
  if( janein() )
    {
    const char* name = getname();
    spektrum* root;
    press( frb );
    clear( frb, zpb/2, zpb-1 );
    root = get_display( frb, name );
    if( root )
      {
      adjust( root );
      outall( frb, root, name );
      }
    }
  }

